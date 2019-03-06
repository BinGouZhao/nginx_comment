#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

#define NGX_INVALID_INDEX  0xd0d0d0d0

struct ngx_event_s {
    void            *data;

    unsigned        write:1;

    unsigned        accept:1;

    unsigned        instance:1;

    unsigned        posted:1;

    unsigned        closed:1;

    ngx_event_handler_pt    handler;

    ngx_queue_t     queue;
};

#define ngx_add_event   ngx_epoll_add_event;
#define ngx_del_event   ngx_epoll_del_event;

static ngx_int_t ngx_epoll_init(ngx_cycle_t *cycle);

extern ngx_uint_t			ngx_event_flags;	


#endif
