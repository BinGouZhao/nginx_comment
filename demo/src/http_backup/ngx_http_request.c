#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_http_request.h>

static void ngx_http_wait_request_handler(ngx_event_t *ev);

void 
ngx_http_empty_handler(ngx_event_t *e)
{
    return;
}

void 
ngx_http_init_connection(ngx_connection_t *c)
{
    ngx_event_t         *rev;

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

    //ngx_add_timer(rev, 1000);
    ngx_reusable_connection(c, 1);

    if (ngx_handle_read_event(rev, 0) != NGX_OK) {
        ngx_http_close_connection(c);
        return;
    }
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
ngx_http_wait_request_handler(ngx_event_t *rev)
{
    size_t                      size;
    ssize_t                     n;
    ngx_buf_t                   *b;
    ngx_connection_t            *c;


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

    size = 512;
    b = c->buffer;

    if (b == NULL) {
        b = ngx_create_temp_buf(c->pool, size);
        if (b == NULL) {
            ngx_http_close_connection(c);
            return;
        }

        c->buffer = b;

    } else if (b->start == NULL) {
        
        b->start = ngx_palloc(c->pool, size);
        if (b->start == NULL) {
            ngx_http_close_connection(c);
            return;
        }

        b->pos = b->start;
        b->last = b->start;
        b->end = b->last + size;
    }

    n = c->recv(c, b->last, size);

    if (n == NGX_AGAIN) {

#if 0
        if (!rev->timer_set) {
            ngx_add_timer(rev, c->listening->post_accept_timeout);
            ngx_reusable_connection(c, 1);
        }
#endif

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
    b->last = '\0';

    c->log->action = "reading client request line";

    ngx_reusable_connection(c, 0);

//    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "recv: %s", b->start);

    char *ret = "HTTP/1.1 200 OK\r\n\r\nhello world";
    n = c->send(c, (u_char *)ret, strlen(ret));
    if ((size_t) n != strlen(ret)) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "send: %d", n);
    }

    ngx_http_close_connection(c);
    return;
#if 0
    c->data = ngx_http_create_request(c);
    if (c->data == NULL) {
        ngx_http_close_connection(c);
        return;
    }

    rev->handler = ngx_http_process_request_line;
    ngx_http_process_request_line(rev);
#endif
}
