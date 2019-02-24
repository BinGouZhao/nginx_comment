#ifndef _NGX_FILES_H_INCLUDED_
#define _NGX_FILES_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

#define NGX_FILE_RDONLY          O_RDONLY
#define NGX_FILE_WRONLY          O_WRONLY
#define NGX_FILE_RDWR            O_RDWR
#define NGX_FILE_CREATE_OR_OPEN  O_CREAT
#define NGX_FILE_OPEN            0
#define NGX_FILE_TRUNCATE        (O_CREAT|O_TRUNC)
#define NGX_FILE_APPEND          (O_WRONLY|O_APPEND)
#define NGX_FILE_NONBLOCK        O_NONBLOCK

#define ngx_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)
#define ngx_open_file_n          "open()"

#define ngx_stderr          STDERR_FILENO
#define ngx_stdout          STDOUT_FILENO

#define NGX_FILE_DEFAULT_ACCESS  0644
#define NGX_FILE_OWNER_ACCESS    0600


#define ngx_close_file           close
#define ngx_close_file_n         "close()"


#define NGX_INVALID_FILE -1
#define NGX_LINEFEED_SIZE        1
#define NGX_LINEFEED             "\x0a"

#define ngx_path_separator(c)    ((c) == '/')

typedef int     ngx_fd_t;

static ngx_inline ssize_t
ngx_write_fd(ngx_fd_t fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

#define ngx_write_console        ngx_write_fd

#define ngx_linefeed(p)     *p++ = LF;

#endif /* _NGX_FILES_H_INCLUDED_ */
