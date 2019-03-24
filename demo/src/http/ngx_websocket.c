#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


//static ngx_int_t ngx_websocket_frame_encode(ngx_buf_t *b, ngx_buf_t *message, char opcode, uint8_t finish); 
static void ngx_websocket_frame_handler(ngx_event_t *wev);

volatile ngx_uint_t         ngx_websocket_message_id;
ngx_websocket_message_t     ngx_websocket_message_send;

#if 0
void
ngx_websocket_init_connection(ngx_connection_t *c)
{
    ngx_websocket_connection_t      *wc;
    ngx_event_t                     *rev;

    wc = ngx_pcalloc(c->pool, sizeof(ngx_websocket_connection_t));
    if (wc == NULL) {
        ngx_http_close_connection(c);
        return;
    }

    // subscribe

    rev = c->read;
    rev->handler = ngx_websocket_wait_frame_handler(ngx_event_t *rev);
    c->write->handler = ngx_http_empty_handler;

    if (rev->ready) {

        if (ngx_use_accept_mutex) {
            ngx_post_event(rev, &ngx_posted_events);
            return;
        }

        rev->handler(rev);
        return;
    }

    ngx_add_timer(rev, c->listening->post_accept_timeout);
    ngx_reusable_connection(c, 1);

    if (ngx_handle_read_event(rev, 0) != NGX_OK) {
        ngx_http_close_connection(c);
        return;
    }
}

void 
ngx_websocket_wait_frame_handler(ngx_event_t *rev)
{
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

void 
ngx_websocket_send(ngx_connection_t *c)
{
    c->write->handler = ngx_websocket_frame_handler;

    ngx_websocket_message_send = ngx_messages[0]; 
    ngx_websocket_message_id = 0;

    if (c->write->ready) {
        ngx_websocket_frame_handler(c->write);
        return;
    } else {
        if (ngx_handle_write_event(c->write, 0) != NGX_OK) {
            ngx_http_close_connection(c);
            return;
        }
    }

    return;
#if 0
    ssize_t         n;
    u_char          *p;
    ngx_buf_t       *b, *message;

    b = c->buffer;

    message = ngx_create_temp_buf(c->pool, 1024);
    if (message == NULL) {
        ngx_http_close_connection(c);
        return;
    }

    b->pos = b->start;
    p = ngx_cpymem(b->pos, "hello", strlen("hello"));
    b->last = p;

    if (ngx_websocket_frame_encode(b, message, NGX_WS_OPCODE_TEXT, 1) != NGX_OK) {
        ngx_http_close_connection(c);
        return;
    }

    n = c->send(c, message->pos, message->last - message->pos);

    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "websocket send %d", n);
#endif
}

static void
ngx_websocket_frame_handler(ngx_event_t *wev)
{
    ssize_t             n;
    size_t              send;
    static u_char       buffer[256];
    ngx_connection_t    *c;

    c = wev->data;

    if (wev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT,
                      "client timed out");
        c->timedout = 1;
        ngx_http_close_connection(c);
        return;
    }

    if (ngx_websocket_message_send.message_id <= ngx_websocket_message_id) {
        return;
    }

    ngx_websocket_message_id = ngx_websocket_message_send.message_id;
    send = strlen((char *)ngx_websocket_message_send.message); 

    if (send <= 0) {
        ngx_http_close_connection(c);
        return;
    }

    ngx_websocket_frame_encode(buffer, ngx_websocket_message_send.message, NGX_WS_OPCODE_TEXT, 1);
    n = c->send(c, buffer, strlen((char *)buffer));

    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "websocket send %d", n);

    if (n == NGX_AGAIN) {

        if (!wev->timer_set) {
//            ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
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

    if ((size_t) n >= send) {
        ngx_websocket_message_send = *ngx_websocket_message_send.next;

        if (!wev->timer_set) {
            //ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            //ngx_reusable_connection(c, 1);
        }

        wev->ready = 0;

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_http_close_connection(c);
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
