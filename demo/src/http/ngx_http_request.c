#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static void ngx_http_wait_request_handler(ngx_event_t *rev);
static void ngx_http_process_request_line(ngx_event_t *rev);
static ssize_t ngx_http_read_request_header(ngx_http_request_t *r);
static void ngx_http_close_request(ngx_http_request_t *r, ngx_int_t rc);
static void ngx_http_process_request_headers(ngx_event_t *rev);
static u_char *ngx_websocket_build_accept_key(ngx_http_request_t *r, u_char *key);
static void ngx_websocket_request_handler(ngx_event_t *ev);
static void ngx_websocket_writer(ngx_http_request_t *r);

static char *ngx_http_client_errors[] = {

    /* NGX_HTTP_PARSE_INVALID_METHOD */
    "client sent invalid method",

    /* NGX_HTTP_PARSE_INVALID_REQUEST */
    "client sent invalid request",

    /* NGX_HTTP_PARSE_INVALID_VERSION */
    "client sent invalid version",

    /* NGX_HTTP_PARSE_INVALID_09_METHOD */
    "client sent invalid method in HTTP/0.9 request"
};

void
ngx_http_empty_handler(ngx_event_t *wev)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, wev->log, 0, "http empty handler");

	return;
}

void
ngx_http_init_connection(ngx_connection_t *c)
{
	ngx_event_t					*rev;
	ngx_http_connection_t		*hc;

	hc = ngx_palloc(c->pool, sizeof(ngx_http_connection_t));
	if (hc == NULL) {
		ngx_http_close_connection(c);
		return;
	}

	c->data = hc;

	c->log->connection = c->number;
//	c->log->handler = ngx_http_log_error;
    c->log->action = "waiting for request";
//  c->log_error = NGX_ERROR_INFO;

	rev = c->read;
	rev->handler = ngx_http_wait_request_handler;
	c->write->handler = ngx_http_empty_handler;

	if (rev->ready) {

		if (ngx_use_accept_mutex) {
			ngx_post_event(rev, &ngx_posted_events);
			return;
		}

		rev->handler(rev);
		return;
	}
	
	//ngx_add_timer(rev, c->listening->post_accept_timeout);
//	ngx_add_timer(rev, 1000);
	ngx_reusable_connection(c, 1);
	
	if (ngx_handle_read_event(rev, 0) != NGX_OK) {
		ngx_http_close_connection(c);
		return;
	}
}

static void 
ngx_http_wait_request_handler(ngx_event_t *rev)
{
	ssize_t					n;
	ngx_buf_t				*b;
	ngx_connection_t		*c;

	c = rev->data;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http wait request handler");

	if (rev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
        ngx_http_close_connection(c);
        return;
    }

	if (c->close) {
		ngx_http_close_connection(c);
		return;
	}

	b = c->buffer;

	if (b == NULL) {
		b = ngx_create_temp_buf(c->pool, NGX_CLIENT_HEADER_BUFFER_SIZE);
		if (b == NULL) {
			ngx_http_close_connection(c);
			return;
		}

		c->buffer = b;

	} else if (b->start == NULL) {

		b->start = ngx_palloc(c->pool, NGX_CLIENT_HEADER_BUFFER_SIZE);
		if (b->start == NULL) {
			ngx_http_close_connection(c);
			return;
		}

		b->pos = b->start;
		b->last = b->start;
		b->end = b->start + NGX_CLIENT_HEADER_BUFFER_SIZE;
	}

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

		if (ngx_pfree(c->pool, b->start) == NGX_OK) {
			b->start = NULL;
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

	c->log->action = "reading client request line";

	ngx_reusable_connection(c, 0);

	c->data = ngx_http_create_request(c);
	if (c->data == NULL) {
		ngx_http_close_connection(c);
		return;
	}

	rev->handler = ngx_http_process_request_line;
	ngx_http_process_request_line(rev);
}

static void 
ngx_http_process_request_line(ngx_event_t *rev)
{
	ssize_t					n;
	ngx_int_t				rc;
	ngx_connection_t		*c;
	ngx_http_request_t		*r;

	c = rev->data;
	r = c->data;
	
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0,
                   "http process request line");

	if (rev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		c->timedout = 1;
		ngx_http_close_request(r, NGX_HTTP_REQUEST_TIME_OUT);
		return;
	}

	rc = NGX_AGAIN;

	for ( ;; ) {

		if (rc == NGX_AGAIN) {
			n = ngx_http_read_request_header(r);

			if (n == NGX_AGAIN || n== NGX_ERROR) {
				return;
			}
		}

		rc = ngx_http_parse_request_line(r, r->header_in);

		if (rc == NGX_OK) {

			r->request_line.len = r->request_end - r->request_start;
			r->request_line.data = r->request_start;
			r->request_length = r->header_in->pos - r->request_start;

			ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
					"http request line: \"%V\"", &r->request_line);

			if (r->http_protocol.data) {
				r->http_protocol.len = r->request_end - r->http_protocol.data;
			}
#if 0
			if (ngx_http_process_request_uri(r) != NGX_OK) {
				return;
			}

			if (r->http_version < NGX_HTTP_VERSION_10) {

				if (r->headers_in.server.len == 0
						&& ngx_http_set_virtual_server(r, &r->headers_in.server)
						== NGX_ERROR)
				{
					return;
				}

				ngx_http_process_request(r);
				return;
			}
#endif

			if (ngx_list_init(&r->headers_in.headers, r->pool, 20,
						sizeof(ngx_table_elt_t))
					!= NGX_OK)
			{
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return;
			}

			c->log->action = "reading client request headers";

			rev->handler = ngx_http_process_request_headers;
			ngx_http_process_request_headers(rev);

			return;
		}

		if (rc != NGX_AGAIN) {

			/* there was error while a request line parsing */

			ngx_log_error(NGX_LOG_INFO, c->log, 0,
					ngx_http_client_errors[rc - NGX_HTTP_CLIENT_ERROR]);

			if (rc == NGX_HTTP_PARSE_INVALID_VERSION) {
				ngx_http_finalize_request(r, NGX_HTTP_VERSION_NOT_SUPPORTED);

			} else {
				ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
			}

			return;
		}

		/* NGX_AGAIN: a request line parsing is still incomplete */

		if (r->header_in->pos == r->header_in->end) {
            ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
#if 0
			rv = ngx_http_alloc_large_header_buffer(r, 1);

			if (rv == NGX_ERROR) {
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return;
			}

			if (rv == NGX_DECLINED) {
				r->request_line.len = r->header_in->end - r->request_start;
				r->request_line.data = r->request_start;

				ngx_log_error(NGX_LOG_INFO, c->log, 0,
						"client sent too long URI");
				ngx_http_finalize_request(r, NGX_HTTP_REQUEST_URI_TOO_LARGE);
				return;
			}
#endif
		}
	}
}

static ssize_t
ngx_http_read_request_header(ngx_http_request_t *r)
{
	ssize_t				n;
	ngx_event_t			*rev;
	ngx_connection_t	*c;

	c = r->connection;
	rev = c->read;

	n = r->header_in->last - r->header_in->pos;

	if (n > 0) {
		return n;
	}

	if (rev->ready) {
		n = c->recv(c, r->header_in->last,
				r->header_in->end - r->header_in->last);
	} else {
		n = NGX_AGAIN;
	}

	if (n == NGX_AGAIN) {
		if (!rev->timer_set) {
			//ngx_add_timer(rev, NGX_CLIENT_HEADER_TIMEOUT);
		}

		if (ngx_handle_read_event(rev, 0) != NGX_OK) {
			ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
			return NGX_ERROR;
		}

		return NGX_AGAIN;
	}

	if (n == 0) {
		ngx_log_error(NGX_LOG_INFO, c->log, 0,
				"client prematurely closed connection");
	}

	if (n == 0 || n == NGX_ERROR) {
		c->error = 1;
		c->log->action = "reading client request headers";

		//ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        ngx_http_close_request(r, NGX_HTTP_BAD_REQUEST);
		return NGX_ERROR;
	}

	r->header_in->last += n;

	return n;
}

static void
ngx_http_process_request_headers(ngx_event_t *rev)
{
	ssize_t					n;
	ngx_connection_t		*c;
	ngx_http_request_t		*r;
	ngx_int_t				rc; //, rv;
    ngx_table_elt_t         *h;

	c = rev->data;
	r = c->data;

	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0,
			"http process request header line");

	if (rev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
		c->timedout = 1;
		ngx_http_close_request(r, NGX_HTTP_REQUEST_TIME_OUT);
		return;
	}

	rc = NGX_AGAIN;

	for ( ;; ) {

		if (rc == NGX_AGAIN) {

			if (r->header_in->pos == r->header_in->end) {

				// todo
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
			}

			n = ngx_http_read_request_header(r);

			if (n == NGX_AGAIN || n == NGX_ERROR) {
				return;
			}
		}

		rc = ngx_http_parse_header_line(r, r->header_in, 1);
        
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "parse header: %d", rc);
        
		if (rc == NGX_OK) {

			r->request_length += r->header_in->pos - r->header_name_start;

			if (r->invalid_header) {

				/* there was error while a header line parsing */

				ngx_log_error(NGX_LOG_INFO, c->log, 0,
						"client sent invalid header line: \"%*s\"",
						r->header_end - r->header_name_start,
						r->header_name_start);
				continue;
			}

			/* a header line has been parsed successfully */

			h = ngx_list_push(&r->headers_in.headers);
			if (h == NULL) {
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return;
			}

			h->hash = r->header_hash;

			h->key.len = r->header_name_end - r->header_name_start;
			h->key.data = r->header_name_start;
			h->key.data[h->key.len] = '\0';

			h->value.len = r->header_end - r->header_start;
			h->value.data = r->header_start;
			h->value.data[h->value.len] = '\0';

			h->lowcase_key = ngx_pnalloc(r->pool, h->key.len);
			if (h->lowcase_key == NULL) {
				ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
				return;
			}

			if (h->key.len == r->lowcase_index) {
				ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);

			} else {
				ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
			}

            // 升级协议到websocket
            if (ngx_strncmp(h->lowcase_key, "upgrade", strlen("upgrade")) == 0 && 
                ngx_strncmp(h->value.data, "websocket", strlen("websocket")) == 0)
            {
                r->upgrade = 1;
            }

            // 验证 key
            if (ngx_strncmp(h->lowcase_key, "sec-websocket-key", 
                strlen("sec-websocket-key")) == 0) 
            {
                r->sec_websocket_key = h->value.data;
            }
#if 0
			hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash,
					h->lowcase_key, h->key.len);

			if (hh && hh->handler(r, h, hh->offset) != NGX_OK) {
				return;
			}
#endif
			ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
					"http header: \"%V: %V\"",
					&h->key, &h->value);

			continue;
		}

		if (rc == NGX_HTTP_PARSE_HEADER_DONE) {

			/* a whole header has been parsed successfully */

			ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
					"http header done");

			r->request_length += r->header_in->pos - r->header_name_start;

			r->http_state = NGX_HTTP_PROCESS_REQUEST_STATE;

#if 0
			rc = ngx_http_process_request_header(r);

			if (rc != NGX_OK) {
                break;
			}
#endif

			ngx_http_process_request(r);
            break;
		}

		if (rc == NGX_AGAIN) {

			/* a header line parsing is still not complete */

			continue;
		}

		/* rc == NGX_HTTP_PARSE_INVALID_HEADER */

		ngx_log_error(NGX_LOG_INFO, c->log, 0,
				"client sent invalid header line");

		ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
		return;
	}

    //ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
}

void 
ngx_http_finalize_request(ngx_http_request_t *r, ngx_int_t rc) 
{
    ngx_connection_t    *c;
    static char         *response = "HTTP/1.1 403 Forbidden\r\n";

    c = r->connection;

    if (rc == NGX_OK) {
        ngx_http_close_request(r, NGX_OK);
        return;
    }

    if (rc == NGX_ERROR
        || rc == NGX_HTTP_REQUEST_TIME_OUT
        || rc == NGX_HTTP_CLIENT_CLOSED_REQUEST
        || c->error)
    {
        ngx_http_close_request(r, NGX_ERROR);
        return;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        r->error = 1;

        sprintf((char *)r->buffer->start, response);

        r->buffer->pos = r->buffer->start;
        r->buffer->last = r->buffer->pos + ngx_strlen(r->buffer->start);

        ngx_websocket_writer(r);
        return;
    }

    ngx_http_close_request(r, NGX_DONE);
    return;
}


void
ngx_http_process_request(ngx_http_request_t *r)
{
	ngx_connection_t		*c;

	c = r->connection;

	if (c->read->timer_set) {
//		ngx_del_timer(c->read);
	}

    c->read->handler = ngx_http_empty_handler;
    c->write->handler = ngx_websocket_request_handler;
    
    //ngx_http_handler(r);
    ngx_websocket_handler(r);
}

void 
ngx_websocket_handler(ngx_http_request_t *r) 
{
    u_char              *sec_websocket_accept;
    static char         *ws_accept = "HTTP/1.1 101 Switching Protocols\r\n"		\
                                      "Upgrade: websocket\r\n"				    \
                                      "Connection: Upgrade\r\n"				    \
                                      "Sec-WebSocket-Accept: %s\r\n"			\
                                      "Sec-WebSocket-Version: 13\r\n\r\n";               

    if (!r->upgrade || r->sec_websocket_key == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }

    sec_websocket_accept = ngx_websocket_build_accept_key(r, r->sec_websocket_key);
    if (sec_websocket_accept == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    sprintf((char *)r->buffer->start, ws_accept, sec_websocket_accept);

    r->buffer->pos = r->buffer->start;
    r->buffer->last = r->buffer->pos + ngx_strlen(r->buffer->start);

    ngx_websocket_writer(r);
    return;
}

static u_char *
ngx_websocket_build_accept_key(ngx_http_request_t *r, u_char *key)
{
	u_char          digest[20];
	ngx_str_t       encoded, decoded;
	ngx_sha1_t      sha1;

	decoded.len = sizeof(digest);
	decoded.data = digest;

	ngx_sha1_init(&sha1);
	ngx_sha1_update(&sha1, key, ngx_strlen(key));
	ngx_sha1_update(&sha1, WEBSOCKET_UUID, sizeof(WEBSOCKET_UUID) - 1);
	ngx_sha1_final(digest, &sha1);

	encoded.len = ngx_base64_encoded_length(decoded.len) + 1;
	encoded.data = ngx_pnalloc(r->pool, encoded.len);

	if (encoded.data == NULL) {
		return NULL;
	}
	ngx_memzero(encoded.data, encoded.len);

	ngx_encode_base64(&encoded, &decoded);

	return encoded.data;
}

ngx_http_request_t *
ngx_http_create_request(ngx_connection_t *c)
{
	ngx_pool_t				*pool;
    ngx_time_t              *tp;
	ngx_http_request_t		*r;
	ngx_http_connection_t	*hc;

	c->requests++;

	hc = c->data;

	pool = ngx_create_pool(NGX_REQUEST_POOL_SIZE, c->log);
	if (pool == NULL) {
		return NULL;
	}

	r = ngx_pcalloc(pool, sizeof(ngx_http_request_t));
	if (r == NULL) {
		ngx_destroy_pool(pool);
		return NULL;
	}

	r->pool = pool;
	r->http_connection = hc;
	r->connection = c;

//	r->read_event_handler = ngx_http_block_reading;

    //ngx_set_connection_log(r->connection, clcf->error_log);
	
	r->header_in = c->buffer;
    r->buffer = c->buffer;
#if 0
   if (ngx_list_init(&r->headers_out.headers, r->pool, 20,
                      sizeof(ngx_table_elt_t))
        != NGX_OK)
    {
        ngx_destroy_pool(r->pool);
        return NULL;
    }

    if (ngx_list_init(&r->headers_out.trailers, r->pool, 4,
                      sizeof(ngx_table_elt_t))
        != NGX_OK)
    {
        ngx_destroy_pool(r->pool);
        return NULL;
    }

    r->variables = ngx_pcalloc(r->pool, cmcf->variables.nelts
                                        * sizeof(ngx_http_variable_value_t));
    if (r->variables == NULL) {
        ngx_destroy_pool(r->pool);
        return NULL;
    }
#endif 

	//r->main = r;
	r->count = 1;

	tp = ngx_timeofday();
	r->start_sec = tp->sec;
	r->start_msec = tp->msec;

	r->method = NGX_HTTP_UNKNOWN;
	r->http_version = NGX_HTTP_VERSION_10;

    r->http_state = NGX_HTTP_READING_REQUEST_STATE;
    
    return r;
}

static void
ngx_http_close_request(ngx_http_request_t *r, ngx_int_t rc)
{
    ngx_connection_t  *c;

    c = r->connection;

    if (rc == NGX_OK) {
        ngx_http_free_request(r, rc);
        return;
    }

    if (rc == NGX_ERROR
        || rc == NGX_HTTP_REQUEST_TIME_OUT
        || rc == NGX_HTTP_CLIENT_CLOSED_REQUEST
        || c->error)
    {
        ngx_http_free_request(r, rc);
        ngx_http_close_connection(c);
        return;
    }

    ngx_http_free_request(r, rc);
    ngx_http_close_connection(c);
}

void 
ngx_http_free_request(ngx_http_request_t *r, ngx_int_t rc)
{
    ngx_log_t       *log;
    ngx_pool_t      *pool;

    log = r->connection->log;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0, "http close request");

    if (r->pool == NULL) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "http request already closed");
        return;
    }
#if 0
    if (rc > 0 && (r->headers_out.status == 0 || r->connection->sent == 0)) {
        r->headers_out.status = rc;
    }
#endif

    log->action = "logging request";

    //ngx_http_log_request(r);

    log->action = "closing request";

    r->request_line.len = 0;

    r->connection->destroyed = 1;

    /*
     * Setting r->pool to NULL will increase probability to catch double close
     * of request since the request object is allocated from its own pool.
     */

    pool = r->pool;
    r->pool = NULL;

    ngx_destroy_pool(pool);
}

void 
ngx_http_close_connection(ngx_connection_t *c)
{
    ngx_pool_t      *pool;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "close http connection: %d", c->fd);

    c->destroyed = 1;

    pool = c->pool;

    ngx_close_connection(c);

    ngx_destroy_pool(pool);
}

static void
ngx_websocket_writer(ngx_http_request_t *r)
{
    ssize_t             n, send;
    ngx_int_t           rc;
    ngx_connection_t    *c;
    ngx_event_t         *wev;

    c = r->connection;
    wev = c->write;

    if (wev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT,
                      "client timed out");
        c->timedout = 1;

        ngx_http_finalize_request(r, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    rc = r->error ? NGX_DONE : NGX_OK;
    send = r->buffer->last - r->buffer->pos;

    if (send <= 0) {
        
        ngx_http_finalize_request(r, rc);
        return;
    }

    n = c->send(c, r->buffer->pos, (size_t) send);

    if (n == NGX_AGAIN) {

        if (!wev->timer_set) {
//            ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        return;
    }

    if (n == NGX_ERROR) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    if (n == 0) {
        ngx_log_error(NGX_LOG_INFO, c->log, 0,
                "client closed connection");
        ngx_http_finalize_request(r, NGX_HTTP_CLIENT_CLOSED_REQUEST);
        return;
    }

    r->buffer->pos += n;

    if (n >= send) {
        ngx_http_close_request(r, rc);

		ngx_websocket_init_connection(c, 1);
        return;

    } else {

        if (!wev->timer_set) {
//            ngx_add_timer(wev, NGX_SEND_RESPONSE_TIMEOUT);
            ngx_reusable_connection(c, 1);
        }

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    return;
}

static void
ngx_websocket_request_handler(ngx_event_t *ev)
{
    ngx_connection_t    *c;
    ngx_http_request_t  *r;

    c = ev->data;
    r = c->data;

    if (c->close) {
        ngx_http_finalize_request(r, 0);
        return;
    }

    if (ev->write) {
        ngx_websocket_writer(r);
    }
}
