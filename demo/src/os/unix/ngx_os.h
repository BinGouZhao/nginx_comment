#ifndef _NGX_OS_H_INCLUDED_
#define _NGX_OS_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

typedef ssize_t (*ngx_recv_pt)(ngx_connection_t *c, u_char *buf, size_t size);
typedef ssize_t (*ngx_send_pt)(ngx_connection_t *c, u_char *bug, size_t size);

ngx_int_t ngx_daemon(ngx_log_t *log);
ssize_t ngx_unix_recv(ngx_connection_t *c, u_char *buf, size_t size);
ssize_t ngx_unix_send(ngx_connection_t *c, u_char *buf, size_t size);

#endif
