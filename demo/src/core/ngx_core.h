#ifndef _NGX_CORE_H_INCLUDED_
#define _NGX_CORE_H_INCLUDED_

#include <ngx_config.h>

typedef struct ngx_cycle_s           ngx_cycle_t;
typedef struct ngx_pool_s            ngx_pool_t;
typedef struct ngx_log_s             ngx_log_t;
typedef struct ngx_open_file_s       ngx_open_file_t;
typedef struct ngx_connection_s      ngx_connection_t;
typedef struct ngx_file_s            ngx_file_t;

//typedef void (*ngx_event_handler_pt)(ngx_event_t *ev);
typedef void (*ngx_connection_handler_pt)(ngx_connection_t *c);

#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6

#include <nginx.h>
#include <ngx_errno.h>
#include <ngx_atomic.h>
#include <ngx_rbtree.h>
#include <ngx_time.h>
#include <ngx_socket.h>
#include <ngx_string.h>
#include <ngx_files.h>
#include <ngx_alloc.h>
#include <ngx_palloc.h>
#include <ngx_queue.h>
#include <ngx_array.h>
#include <ngx_file.h>
#include <ngx_process.h>
#include <ngx_log.h>
#include <ngx_conf_file.h>
#include <ngx_cycle.h>
#include <ngx_times.h>
#include <ngx_connection.h>
#include <ngx_process_cycle.h>
#include <ngx_os.h>

#define ngx_cdecl

#define LF     (u_char) '\n'
#define ngx_abs(value)       (((value) >= 0) ? (value) : - (value))
#define ngx_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define ngx_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#endif /* _NGX_CORE_H_INCLUDED_ */
