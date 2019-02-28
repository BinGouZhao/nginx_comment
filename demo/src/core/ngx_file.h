#ifndef _NGX_FILE_H_INCLUDED_
#define _NGX_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


struct ngx_file_s {
    ngx_fd_t            fd;
    ngx_str_t           name;
    ngx_file_info_t     info;

    off_t               offset;
    off_t               sys_offset;

    ngx_log_t           *log;
};

#endif

