#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define NGX_CLOSE_EVENT         1
#define NGX_DISABLE_EVENT       2
#define NGX_POST_EVENTS         2

#define NGX_USE_CLEAR_EVENT      0x00000004
#define NGX_USE_EPOLL_EVENT      0x00000040
#define NGX_USE_GREEDY_EVENT     0x00000020

#define NGX_INVALID_INDEX  0xd0d0d0d0

#define NGX_UPDATE_TIME         1

struct ngx_event_s {
    void            *data;

    unsigned        write:1;
    unsigned        ready:1;
    unsigned        timedout:1;

    unsigned        accept:1;

    unsigned        instance:1;

    unsigned        posted:1;

    unsigned        eof:1;
    unsigned        error:1;
                    
    unsigned        pending_eof:1;

    unsigned        closed:1;

    unsigned        active:1;
    unsigned        disabled:1;

    unsigned        available:1;
    unsigned        timer_set:1;

    unsigned         cancelable:1;

    ngx_log_t       *log;

    ngx_event_handler_pt    handler;

    ngx_uint_t      index;

    ngx_rbtree_node_t   timer;
    ngx_queue_t     queue;
};

#define ngx_add_event   ngx_epoll_add_event
#define ngx_del_event   ngx_epoll_del_event
#define ngx_del_conn    ngx_epoll_del_connection

#define ngx_del_timer        ngx_event_del_timer

#define NGX_READ_EVENT     EPOLLIN
#define NGX_WRITE_EVENT    EPOLLOUT
#define NGX_CLEAR_EVENT    EPOLLET

ngx_int_t ngx_epoll_init(ngx_cycle_t *cycle);
ngx_int_t ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);
ngx_int_t ngx_epoll_del_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);
ngx_int_t ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags);
ngx_int_t ngx_epoll_del_connection(ngx_connection_t *c, ngx_uint_t flags);

void ngx_event_accept(ngx_event_t *ev);
void ngx_process_events_and_timers(ngx_cycle_t *cycle);
ngx_int_t ngx_event_process_init(ngx_cycle_t *cycle);

ngx_int_t ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags);

ngx_int_t ngx_enable_accept_events(ngx_cycle_t *cycle);

extern ngx_uint_t           ngx_use_accept_mutex;
extern ngx_uint_t           ngx_accept_mutex_held;
extern ngx_msec_t           ngx_accept_mutex_delay;
extern ngx_uint_t			ngx_event_flags;	
extern ngx_int_t            ngx_accept_disabled;

extern ngx_uint_t           ngx_use_epoll_rdhup;

#define ngx_event_ident(p)  ((ngx_connection_t *) (p))->fd

#include <ngx_event_timer.h>
#include <ngx_event_posted.h>


#endif
