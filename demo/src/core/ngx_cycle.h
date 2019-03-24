#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif

#define NGX_WEBSOCKET_MESSAGE_N             1024
#define NGX_WEBSOCKET_MAX_MESSAGE_LENGTH    256

struct ngx_cycle_s {
    ngx_pool_t              *pool;

    ngx_log_t               *log;
    ngx_log_t               new_log;

    ngx_uint_t              log_use_stderr;  /* unsigned  log_use_stderr:1; */

    ngx_cycle_t             *old_cycle;

    ngx_array_t             listening;

    ngx_connection_t        **files;
    ngx_connection_t        *free_connections;
    ngx_uint_t              free_connection_n;

    ngx_connection_t        *connections;
    ngx_uint_t              connection_n;

    ngx_event_t             *read_events;
    ngx_event_t             *write_events;

    ngx_queue_t             reusable_connections_queue;
    ngx_uint_t              reusable_connections_n;

    ngx_str_t               hostname;
};

struct ngx_websocket_message_s {
    ngx_uint_t          message_id;
    ngx_uint_t          channel_id;

    time_t              start_sec;
    ngx_msec_t          start_msec;

    u_char              message[NGX_WEBSOCKET_MAX_MESSAGE_LENGTH];
    ngx_uint_t          message_length;

    ngx_websocket_message_t *next;
};

ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);

extern volatile ngx_cycle_t  *ngx_cycle;

extern ngx_websocket_message_t    *ngx_messages;
extern volatile ngx_uint_t        ngx_message_index;
extern volatile ngx_uint_t        ngx_message_id;

#endif
