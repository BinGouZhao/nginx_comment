#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

#include <ngx_setproctitle.h>

typedef pid_t          ngx_pid_t;

#define NGX_INVALID_PID -1

typedef void (*ngx_spawn_proc_pt) (ngx_cycle_t *cycle, void *data);

typedef struct {
    ngx_pid_t           pid;
    int                 status;
    ngx_socket_t        channel[2];

    ngx_spawn_proc_pt   proc;
    void                *data;
    char                *name;

    unsigned            respawn:1;
    unsigned            just_spawn:1;
    unsigned            detached:1;
    unsigned            exiting:1;
    unsigned            exited:1;
} ngx_process_t;

#define NGX_MAX_PROCESSES         1024

#define NGX_PROCESS_NORESPAWN     -1
#define NGX_PROCESS_JUST_SPAWN    -2
#define NGX_PROCESS_RESPAWN       -3
#define NGX_PROCESS_JUST_RESPAWN  -4
#define NGX_PROCESS_DETACHED      -5


#define ngx_getpid   getpid
#define ngx_getppid  getppid

#ifndef ngx_log_pid
#define ngx_log_pid  ngx_pid
#endif

ngx_int_t   ngx_init_signals(ngx_log_t *log);

extern int            ngx_argc;
extern char         **ngx_argv;
extern char         **ngx_os_argv;

#endif
