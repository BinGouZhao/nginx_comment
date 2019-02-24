#ifndef _NGX_CONF_FILE_H_INCLUDED_
#define _NGX_CONF_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

struct ngx_open_file_s {
    ngx_fd_t        fd;
    ngx_str_t       name;

    void            (*flush)(ngx_open_file_t *file, ngx_log_t *log);
    void            *data;
};

#endif /* _NGX_CONF_FILE_H_INCLUDED_ */
