#ifndef _NGX_HTTP_REQUEST_H_INCLUDED_
#define _NGX_HTTP_REQUEST_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

void ngx_http_empty_handler(ngx_event_t *e);

void ngx_http_init_connection(ngx_connection_t *c);

void ngx_http_close_connection(ngx_connection_t *c);

#endif
