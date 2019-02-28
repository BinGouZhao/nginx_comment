#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;

struct ngx_listening_s {
    ngx_socket_t        fd;

    struct sockaddr     *sockaddr;
    socklen_t           socklen;
    size_t              addr_text_max_len;
    ngx_str_t           addr_text;

    int                 type;
    int                 backlog;
    int                 rcvbuf;
    int                 sndbuf;

    ngx_log_t           log;

    unsigned            listen:1;
    unsigned            keepalive:2;
};

struct ngx_connection_s {
    ngx_socket_t        fd;
};

ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
ngx_listening_t *ngx_create_listening(ngx_cycle_t *cycle, struct sockaddr *sockaddr, socklen_t socklen);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
 
