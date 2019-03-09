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

    size_t              pool_size;

    ngx_connection_t    *connection;

    unsigned            listen:1;
    unsigned            keepalive:2;
};

struct ngx_connection_s {
    void                *data;
    ngx_event_t         *read;
    ngx_event_t         *write;

    ngx_socket_t        fd;

    ngx_recv_pt         recv;
    ngx_send_pt         send;

    ngx_listening_t     *listening;
    
    ngx_pool_t          *pool;
    ngx_log_t           *log;

    int                 type;

    struct sockaddr     *sockaddr;
    socklen_t           socklen;

    struct sockaddr     *local_sockaddr;
    socklen_t           local_socklen;

    ngx_queue_t         queue;

    ngx_atomic_uint_t   number;

    unsigned            close:1;
};

ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
ngx_listening_t *ngx_create_listening(ngx_cycle_t *cycle, struct sockaddr *sockaddr, socklen_t socklen);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);

ngx_connection_t *ngx_get_connection(ngx_socket_t fd, ngx_log_t *log);
void ngx_free_connection(ngx_connection_t *c);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
 
