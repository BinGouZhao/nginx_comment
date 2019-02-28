#include <ngx_config.h>
#include <ngx_core.h>

ngx_listening_t *
ngx_create_listening(ngx_cycle_t *cycle, struct sockaddr *sockaddr,
    socklen_t socklen)
{
    ngx_listening_t     *ls;
    struct sockaddr     *sa;
        
    ls = ngx_array_push(&(cycle->listening));

    ngx_memzero(ls, sizeof(ngx_listening_t));

    sa = ngx_palloc(cycle->pool, socklen);
    if (sa == NULL) {
        return NULL;
    }
    ngx_memcpy(sa, sockaddr, socklen);

    ls->sockaddr = sa;
    ls->socklen = socklen;

    /*
    len = ngx_sock_ntop(sa, socklen, text, NGX_SOCKADDR_STRLEN, 1);
    ls->addr_text.len = len;
    ls->addr_text_max_len = NGX_UNIX_ADDRSTRLEN;
    len++;
    ls->addr_text.data = ngx_pnalloc(cf->pool, len);
    if (ls->addr_text.data == NULL) {
        return NULL;
    }
    ngx_memcpy(ls->addr_text.data, text, len);
    */

    ls->fd = (ngx_socket_t) -1;
    ls->type = SOCK_STREAM;

    ls->backlog = 511;//NGX_LISTEN_BACKLOG;

    return ls;
}

ngx_int_t
ngx_open_listening_sockets(ngx_cycle_t *cycle) 
{
    ngx_log_t       *log;
    ngx_uint_t      tries, failed, i;
    ngx_err_t       err;
    ngx_socket_t    s;
    ngx_listening_t *ls;

    log = cycle->log;

    for (tries = 5; tries; tries++) {
        failed = 0;

        ls = cycle->listening.elts;
        for (i = 0; i < cycle->listening.nelts; i++) {

            if (ls[i].fd != (ngx_socket_t) -1) {
                continue;
            }

            s = ngx_socket(ls[i].sockaddr->sa_family, ls[i].type, 0);
            if (s == (ngx_socket_t) -1) {
                ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                        "socket() %V failed", &ls[i].addr_text);
                return NGX_ERROR;
            }

            int reuseaddr = 1;
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                        (const void *) &reuseaddr, sizeof(int))
                    == -1)
            {
                ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                        "setsockopt(SO_REUSEADDR) %V failed",
                        &ls[i].addr_text);

                if (ngx_close_socket(s) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                            ngx_close_socket_n " %V failed",
                            &ls[i].addr_text);
                }

                return NGX_ERROR;
            }

            int reuseport = 1;
            if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT,
                        (const void *)&reuseport, sizeof(int))
                    == -1)
            {
                ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                        "setsockopt(SO_REUSEPORT) %V failed",
                        &ls[i].addr_text);

                if (ngx_close_socket(s) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                            ngx_close_socket_n " %V failed",
                            &ls[i].addr_text);
                }

                return NGX_ERROR;
            }

            // 非阻塞
            if (ngx_nonblocking(s) == -1) {
                ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                        ngx_nonblocking_n " %V failed",
                        &ls[i].addr_text);

                if (ngx_close_socket(s) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                            ngx_close_socket_n " %V failed",
                            &ls[i].addr_text);
                }

                return NGX_ERROR;
            }

            if (bind(s, ls[i].sockaddr, ls[i].socklen) == -1) {
                err = ngx_socket_errno;

                if (err != NGX_EADDRINUSE) {
                    ngx_log_error(NGX_LOG_EMERG, log, err,
                            "bind() to %V failed", &ls[i].addr_text);
                }

                if (ngx_close_socket(s) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                            ngx_close_socket_n " %V failed",
                            &ls[i].addr_text);
                }

                if (err != NGX_EADDRINUSE) {
                    return NGX_ERROR;
                }

                failed = 1;
                continue;
            }
#if 0
            if (ls[i].sockaddr->sa_family == AF_UNIX) {
                mode_t      mode;
                u_char      *name;

                name = ls[i].addr_text.data + sizeof("unix:") -1;
                mode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

                if (chmod((char *) name, mode) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                            "chmod() \"%s\" failed", name);
                }
            }
#endif
            if (listen(s, ls[i].backlog) == -1) {
                err = ngx_socket_errno;

                if (err != NGX_EADDRINUSE) {
                    ngx_log_error(NGX_LOG_EMERG, log, err,
                            "listen() to %V, backlog %d failed",
                            &ls[i].addr_text, ls[i].backlog);
                }

                if (ngx_close_socket(s) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, log, ngx_socket_errno,
                            ngx_close_socket_n " %V failed",
                            &ls[i].addr_text);
                }

                if (err != NGX_EADDRINUSE) {
                    return NGX_ERROR;
                }

                failed = 1;
                continue;
            }

            ls[i].listen = 1;
            ls[i].fd = s;
        }

        if (!failed) {
            break;
        }
        ngx_log_error(NGX_LOG_NOTICE, log, 0,
                "try again to bind() after 500ms");

        ngx_msleep(500);
    }

    if (failed) {
        ngx_log_error(NGX_LOG_EMERG, log, 0, "still could not bind()");
        return NGX_ERROR;
    }

    return NGX_OK;
}

void 
ngx_configure_listening_sockets(ngx_cycle_t *cycle)
{
    int                 value;
    ngx_uint_t          i;
    ngx_listening_t     *ls;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
       if (ls[i].rcvbuf != -1) {
           if (setsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF,
                          (const void *) &ls[i].rcvbuf, sizeof(int))
               == -1)
           {
               ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno,
                        "setsockopt(SO_RCVBUF, %d) %V failed, ignored",
                              ls[i].rcvbuf, &ls[i].addr_text);
            }
        }

        if (ls[i].sndbuf != -1) {
            if (setsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF,
                           (const void *) &ls[i].sndbuf, sizeof(int))
                == -1)
            {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno,
                              "setsockopt(SO_SNDBUF, %d) %V failed, ignored",
                              ls[i].sndbuf, &ls[i].addr_text);
            }
        }

        if (ls[i].keepalive) {
            value = (ls[i].keepalive == 1) ? 1 : 0;

            if (setsockopt(ls[i].fd, SOL_SOCKET, SO_KEEPALIVE,
                           (const void *) &value, sizeof(int))
                == -1)
            {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno,
                              "setsockopt(SO_KEEPALIVE, %d) %V failed, ignored",
                              value, &ls[i].addr_text);
            }
        }

        if (ls[i].listen) {

            /* change backlog via listen() */

            if (listen(ls[i].fd, ls[i].backlog) == -1) {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_socket_errno,
                        "listen() to %V, backlog %d failed, ignored",
                        &ls[i].addr_text, ls[i].backlog);
            }
        }
    }

    return;
}


