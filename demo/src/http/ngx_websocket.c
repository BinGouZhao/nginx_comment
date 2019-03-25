#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


//static ngx_int_t ngx_websocket_frame_encode(ngx_buf_t *b, ngx_buf_t *message, char opcode, uint8_t finish); 
//static void ngx_websocket_frame_handler(ngx_event_t *wev);
static void ngx_websocket_wait_frame_handler(ngx_event_t *rev);
ngx_uint_t ngx_websocket_channel_hash_func(ngx_uint_t key);
int ngx_websocket_channel_equal_func(ngx_uint_t k1, ngx_uint_t k2);
static void ngx_websocket_send_message_handler(ngx_event_t *wev);
static void ngx_websocket_message_handler(ngx_websocket_connection_t *wc);
void ngx_websocket_close_connection(ngx_websocket_connection_t *wc);


volatile ngx_uint_t         ngx_websocket_message_id_sent;
ngx_websocket_message_t     *ngx_websocket_message_send;

ngx_hash_table_t            *ngx_websocket_hash_table;

ngx_uint_t
ngx_websocket_channel_hash_func(ngx_uint_t key) {
    return key;
}

int
ngx_websocket_channel_equal_func(ngx_uint_t k1, ngx_uint_t k2)
{
    return k1 == k2;
}


ngx_int_t
ngx_websocket_init() {
    ngx_websocket_hash_table = ngx_hash_table_new(ngx_websocket_channel_hash_func,
                                                  ngx_websocket_channel_equal_func);
    if (ngx_websocket_hash_table == NULL) {
        return NGX_ERROR;
    }

	ngx_websocket_message_send = &ngx_messages[0];
	ngx_websocket_message_id_sent = 0;

    return NGX_OK;
}

void
ngx_websocket_init_connection(ngx_connection_t *c, ngx_uint_t channel_id)
{
    ngx_queue_t                 *channel_queue;
    ngx_websocket_connection_t  *wc;
    ngx_pool_t                  *pool;

    pool = ngx_create_pool(512, c->log); 
    if (pool == NULL) {
        ngx_http_close_connection(c);
        return;
    }

    wc = ngx_pcalloc(pool, sizeof(ngx_websocket_connection_t));
    if (wc == NULL) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "malloc (websocket init) failed.");
        ngx_destroy_pool(pool);
        ngx_http_close_connection(c);
        return;
    }

    wc->pool = pool;
    wc->channel_id = channel_id;
    wc->state = NGX_WS_CONNECTION_WRITE_STATE;
    wc->data = c;
    c->data = wc;

    channel_queue = ngx_hash_table_lookup(ngx_websocket_hash_table, channel_id);
    if (channel_queue == ngx_hash_table_null) {
        channel_queue = calloc(1, sizeof(ngx_queue_t));
        if (channel_queue == NULL) {
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "calloc (websocket init) failed.");
            ngx_http_close_connection(c);
            return;
        }

        ngx_queue_init(channel_queue);

        if (ngx_hash_table_insert(ngx_websocket_hash_table, channel_id, channel_queue) == 0) {
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "hash insert (websocket init) failed.");
            ngx_http_close_connection(c);
            return;
        }
    }

    ngx_queue_insert_tail(channel_queue, &wc->queue);

    c->read->handler = ngx_websocket_wait_frame_handler;
    c->write->handler = ngx_websocket_send_message_handler;

/*
    ngx_add_timer(rev, c->listening->post_accept_timeout);
    ngx_reusable_connection(c, 1);
*/
    return;
} 

static void 
ngx_websocket_wait_frame_handler(ngx_event_t *rev)
{
    return;
}
#if 0
	ssize_t				n;
	ngx_int_t			rc;	
	ngx_buf_t			*b;
	ngx_connection_t	*c;

	c = rev->data;

	if (rev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT,
                      "client timed out");
        c->timedout = 1;

        ngx_http_close_connection(c);
        return;
    }

	if (c->close) {
		ngx_http_close_connection(c);
		return;
	}

	b = c->buffer;
	
	n = c->recv(c, b->last, NGX_CLIENT_HEADER_BUFFER_SIZE);

	if (n == NGX_AGAIN) {
		
		if (!rev->timer_set) {
			//ngx_add_timer(rev, NGX_POST_ACCEPT_TIMEOUT);
			ngx_reusable_connection(c, 1);
		}

		if (ngx_handle_read_event(rev, 0) != NGX_OK) {
			ngx_http_close_connection(c);
			return;
		}

		return;
	}

	if (n == NGX_ERROR) {
		ngx_http_close_connection(c);
		return;
	}

	if (n == 0) {
		ngx_log_error(NGX_LOG_INFO, c->log, 0,
				"client closed connection");
		ngx_http_close_connection(c);
		return;
	}

	b->last += n;
	
	c->log->action = "reading client websocket frame";

	ngx_reusable_connection(c, 0);

	rev->handler = ngx_websocket_process_frame;
	ngx_websocket_process_frame(rev);
}

static void 
ngx_websocket_process_frame(ngx_event_t *rev)
{
	ssize_t				n;
	ngx_int_t			rc;
	ngx_connection_t	
}
#endif

static void 
ngx_websocket_send_message_handler(ngx_event_t *wev)
{
	size_t              		size;
	ssize_t             		n;
    ngx_connection_t    		*c;
    ngx_websocket_connection_t 	*wc;


    c = wev->data;
    wc = c->data;
    
	if (wev->timedout) {
	    ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT,
	                  "client timed out");
	    c->timedout = 1;
	    ngx_websocket_close_connection(wc);
	    return;
	}

	size = wc->message_size;

	if (size == 0) {
		return;
	}

    n = c->send(c, wc->buffer, size);
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "websocket send %d", n);

    if (n == NGX_AGAIN) {

        if (!wev->timer_set) {
            //ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_websocket_close_connection(wc);
            return;
        }

        return;
    }

    if (n == NGX_ERROR) {
        ngx_websocket_close_connection(wc);
        return;
    }

    if (n == 0) {
        ngx_log_error(NGX_LOG_INFO, c->log, ngx_errno,
                "client closed connection");
        ngx_websocket_close_connection(wc);
        return;
    }

    if (n < (ssize_t) size) {
        if (!wev->timer_set) {
            //ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            //ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_websocket_close_connection(wc);
            return;
        }
    }

    return;
}

ngx_uint_t
ngx_websocket_frame_encode(u_char *buffer, u_char *message, char opcode, uint8_t finish) 
{
    int             pos;
    u_char          *p;
    size_t          n;
    uint8_t         mask;
    char            frame_header[16];

    pos = 0;
    mask = 0;
    n = strlen((char *)message);

    frame_header[pos++] = WEBSOCKET_FRAME_SET_FIN(finish) | WEBSOCKET_FRAME_SET_OPCODE(opcode);

    if (n < 126) {
        frame_header[pos++] = WEBSOCKET_FRAME_SET_MASK(mask) | WEBSOCKET_FRAME_SET_LENGTH(n, 0);
    } else {
        if (n < 65536) {
            frame_header[pos++] = WEBSOCKET_FRAME_SET_MASK(mask) | 126;
        } else {

            frame_header[pos++] = WEBSOCKET_FRAME_SET_MASK(mask) | 127;
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 7);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 6);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 5);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 4);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 3);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 2);
        }
        frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 1);
        frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 0);
    }

    p = ngx_cpymem(buffer, frame_header, pos);

    if (mask) {
        // pass
    } else {
        p = ngx_cpymem(p, message, n);
    }
    *p = '\0';

    return p - buffer;
}

#if 0
static ngx_int_t
ngx_websocket_frame_encode(ngx_buf_t *b, ngx_buf_t *message, char opcode, uint8_t finish) 
{
    int             pos;
    u_char          *p;
    ssize_t         n;
    uint8_t         mask;
    char            frame_header[16];

    pos = 0;
    mask = 0;
    n = b->last - b->pos;

    frame_header[pos++] = WEBSOCKET_FRAME_SET_FIN(finish) | WEBSOCKET_FRAME_SET_OPCODE(opcode);
    if (n <= 0) {
        return NGX_ERROR;
    }

    if (n < 126) {
        frame_header[pos++] = WEBSOCKET_FRAME_SET_MASK(mask) | WEBSOCKET_FRAME_SET_LENGTH(n, 0);
    } else {
        if (n < 65536) {
            frame_header[pos++] = WEBSOCKET_FRAME_SET_MASK(mask) | 126;
        } else {

            frame_header[pos++] = WEBSOCKET_FRAME_SET_MASK(mask) | 127;
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 7);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 6);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 5);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 4);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 3);
            frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 2);
        }
        frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 1);
        frame_header[pos++] = WEBSOCKET_FRAME_SET_LENGTH(n, 0);
    }

    p = ngx_cpymem(message->pos, frame_header, pos);

    if (mask) {
        // pass
    } else {
        p = ngx_cpymem(p, b->pos, n);
    }
    message->last = p;

    return NGX_OK;
}
#endif

void
ngx_websocket_process_messages(ngx_cycle_t *cycle)
{
    ngx_uint_t                  channel_id;
    ngx_queue_t                 *channel_queue, *next;
    ngx_websocket_connection_t  *wc;


    while (ngx_websocket_message_send->message_id > ngx_websocket_message_id_sent) {
        // todo 消息加锁
        channel_id = ngx_websocket_message_send->channel_id;

        channel_queue = ngx_hash_table_lookup(ngx_websocket_hash_table, channel_id);
        if (channel_queue == ngx_hash_table_null) {
			ngx_websocket_message_id_sent = ngx_websocket_message_send->message_id;
			ngx_websocket_message_send = ngx_websocket_message_send->next;

			continue;
		}
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "process websocket message %d", (int) ngx_websocket_message_send->message_id);

        next = channel_queue->next;
        while (next != channel_queue) {
            wc = ngx_queue_data(next, ngx_websocket_connection_t, queue);

            ngx_websocket_message_handler(wc);

            next = next->next;
        }

        ngx_websocket_message_id_sent = ngx_websocket_message_send->message_id;
        ngx_websocket_message_send = ngx_websocket_message_send->next;
        // todo 消息解锁
    }   
}

void
ngx_websocket_close_connection(ngx_websocket_connection_t *wc)
{
    ngx_connection_t        *c;

    c = wc->data;
    
    ngx_queue_remove(&wc->queue);

    ngx_destroy_pool(wc->pool);
    ngx_http_close_connection(c);
}

static void 
ngx_websocket_message_handler(ngx_websocket_connection_t *wc)
{
	size_t					size;
	ssize_t					n;
    ngx_connection_t        *c;
    ngx_event_t             *wev;

    c = wc->data;
    wev = c->write;

    if (wev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT,
                      "client timed out");
        c->timedout = 1;
        ngx_websocket_close_connection(wc);
        return;
    }

    size = ngx_websocket_message_send->message_length;

    if (size == 0) {
        return;
    }

    n = c->send(c, ngx_websocket_message_send->message, size);
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "websocket send(message handler) :%d bytes", n);

    if (n == NGX_AGAIN) {

        if (!wev->timer_set) {
            //ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_websocket_close_connection(wc);
            return;
        }

        return;
    }

    if (n == NGX_ERROR) {
        ngx_websocket_close_connection(wc);
        return;
    }

    if (n == 0) {
        ngx_log_error(NGX_LOG_INFO, c->log, ngx_errno,
                "client closed connection");
        ngx_websocket_close_connection(wc);
        return;
    }

    if (n < (ssize_t) size) {
        if (!wev->timer_set) {
            //ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            //ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_websocket_close_connection(wc);
            return;
        }
    }

    return;
}
