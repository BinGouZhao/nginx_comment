#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

static ngx_int_t ngx_disable_accept_events(ngx_cycle_t *cycle, ngx_uint_t all);
static void ngx_close_accepted_connection(ngx_connection_t *c);

void
ngx_event_accept(ngx_event_t *ev)
{
    socklen_t          socklen;
    ngx_err_t          err;
    ngx_log_t         *log;
    ngx_uint_t         level;
    ngx_socket_t       s;
    ngx_event_t       *rev, *wev;
    ngx_sockaddr_t     sa;
    ngx_listening_t   *ls;
    ngx_connection_t  *c, *lc;
    //ngx_event_conf_t  *ecf;

//    static ngx_uint_t  use_accept4 = 1;

	if (ev->timedout) {
		if (ngx_enable_accept_events((ngx_cycle_t *) ngx_cycle) != NGX_OK) {
			return;
		}

		ev->timedout = 0;
	}

	ev->available = 1;

	lc = ev->data;
	ls = lc->listening;
	ev->ready = 0;

	ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "accept on %V, ready: %d", &ls->addr_text, ev->available);

	do {
		socklen = sizeof(ngx_sockaddr_t);

		s = accept4(lc->fd, &sa.sockaddr, &socklen, SOCK_NONBLOCK);

		if (s == (ngx_socket_t) -1) {
			err = ngx_socket_errno;

			if (err == NGX_EAGAIN) {
				ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, err,
						       "accept() not ready");
				return;
			}

			level = NGX_LOG_ALERT;

			if (err == NGX_ECONNABORTED) {
				level = NGX_LOG_ERR;

			} else if (err == NGX_EMFILE || err == NGX_ENFILE) {
				level = NGX_LOG_CRIT;
			}

			ngx_log_error(level, ev->log, err, "accept4() failed");

			if (err == NGX_ECONNABORTED) {
				if (ev->available) {
					continue;
				}
			}

			if (err == NGX_EMFILE || err == NGX_ENFILE) {
				if (ngx_disable_accept_events((ngx_cycle_t *) ngx_cycle, 1) 
                        != NGX_OK) 
                {
					return;
				}

				if (ngx_use_accept_mutex) {
					if (ngx_accept_mutex_held) {
						//ngx_shmtx_unlock(&ngx_accept_mutex);
						ngx_accept_mutex_held = 0;
					}

					ngx_accept_disabled = 1;

				} else {
					//ngx_add_timer(ev, 100);
				}
			}

			return;
		}

		ngx_accept_disabled = ngx_cycle->connection_n / 8
							  - ngx_cycle->free_connection_n;

		c = ngx_get_connection(s, ev->log);

		if (c == NULL) {
			if (ngx_close_socket(s) == -1) {
				ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_socket_errno,
							  "close() failed");
			}

			return;
		}

		c->type = SOCK_STREAM;

		c->pool = ngx_create_pool(ls->pool_size, ev->log);
		if (c->pool == NULL) {
			ngx_close_accepted_connection(c);
			return;
		}

		if (socklen > (socklen_t) sizeof(ngx_sockaddr_t)) {
			socklen = sizeof(ngx_sockaddr_t);
		}

		c->sockaddr = ngx_palloc(c->pool, socklen);
		if (c->sockaddr == NULL) {
			ngx_close_accepted_connection(c);
			return;
		}

		ngx_memcpy(c->sockaddr, &sa, socklen);

		log = ngx_palloc(c->pool, sizeof(ngx_log_t));
		if (log == NULL) {
			ngx_close_accepted_connection(c);
			return;
		}

		if (ngx_nonblocking(s) == -1) {
			ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_socket_errno,
						  "ngx_nonblocking() failed");
			ngx_close_accepted_connection(c);
			return;
		}
		
		*log = ls->log;

		//c->recv = ngx_recv;
		//c->send = ngx_send;
		//c->recv_chain = ngx_recv_chain;
		//c->send_chain = ngx_send_chain;

		c->log = log;
		c->pool->log = log;

		c->socklen = socklen;
		c->listening = ls;
		c->local_sockaddr = ls->sockaddr;
		c->local_socklen = ls->socklen;

		rev = c->read;
		wev = c->write;

		wev->ready = 1;

		rev->log = log;
		wev->log = log;

		//c->number = ngx_atomic_fetch_add(ngx_connection_counter, 1);

        log->data = NULL;
		log->handler = NULL;

		//ls->handler(c);
        ngx_close_accepted_connection(c);
	} while (ev->available);
}

static void
ngx_close_accepted_connection(ngx_connection_t *c)
{
	ngx_socket_t		fd;

	ngx_free_connection(c);

	fd = c->fd;
	c->fd = (ngx_socket_t) -1;

	if (ngx_close_socket(fd) == -1) {
		ngx_log_error(NGX_LOG_ALERT, c->log, ngx_socket_errno,
					  "close() failed");
	}

	if (c->pool) {
		ngx_destroy_pool(c->pool);
	}
}

static ngx_int_t
ngx_disable_accept_events(ngx_cycle_t *cycle, ngx_uint_t all)
{
	ngx_uint_t			i;
	ngx_listening_t		*ls;
	ngx_connection_t	*c;

	ls = cycle->listening.elts;
	for (i = 0; i < cycle->listening.nelts; i++) {
		c = ls[i].connection;

		if (c == NULL || !c->read->active) {
			continue;
		}

		if (ngx_del_event(c->read, NGX_READ_EVENT, NGX_DISABLE_EVENT) == NGX_ERROR) {
			return NGX_ERROR;
		}
	}

	return NGX_OK;
}

ngx_int_t
ngx_enable_accept_events(ngx_cycle_t *cycle) 
{
	ngx_uint_t			i;
	ngx_listening_t		*ls;
	ngx_connection_t	*c;

	ls = cycle->listening.elts;
	for (i = 0; i < cycle->listening.nelts; i++) {
		
		c = ls[i].connection;

		if (c == NULL || c->read->active) {
			continue;
		}

		if (ngx_add_event(c->read, NGX_READ_EVENT, 0) == NGX_ERROR) {
			return NGX_ERROR;
		}
	}

	return NGX_OK;
}

