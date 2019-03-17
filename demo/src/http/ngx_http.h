#ifndef _NGX_HTTP_H_INCLUDED_
#define _NGX_HTTP_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

typedef struct ngx_http_request_s ngx_http_request_t;

#include <ngx_http_request.h>

void ngx_http_process_request(ngx_http_request_t *r);

void ngx_http_empty_handler(ngx_event_t *wev);
void ngx_websocket_handler(ngx_http_request_t *r);
ngx_http_request_t *ngx_http_create_request(ngx_connection_t *c);

void ngx_http_init_connection(ngx_connection_t *c);
void ngx_http_close_connection(ngx_connection_t *c);
void ngx_http_finalize_request(ngx_http_request_t *r, ngx_int_t rc);

ngx_int_t ngx_http_parse_request_line(ngx_http_request_t *r, ngx_buf_t *b);
ngx_int_t ngx_http_parse_header_line(ngx_http_request_t *r, ngx_buf_t *b,
    ngx_uint_t allow_underscores);

void ngx_http_free_request(ngx_http_request_t *r, ngx_int_t rc);
#endif
