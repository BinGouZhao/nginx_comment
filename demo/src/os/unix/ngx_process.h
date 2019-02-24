#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

#include <ngx_setproctitle.h>

typedef pid_t         ngx_pid_t;
extern int            ngx_argc;
extern char         **ngx_argv;
extern char         **ngx_os_argv;

#endif
