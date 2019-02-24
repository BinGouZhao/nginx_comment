#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;

struct ngx_listening_s {
    ngx_socket_t        fd;

    struct sockaddr     *sockaddr;
    socklen_t           socklen;

    int                 backlog;
};

struct ngx_connection_s {
    ngx_socket_t        fd;
};

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
 
