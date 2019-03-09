#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

static int					notify_fd = -1;
//ngx_event_t					notify_event;
//static ngx_connection_t		notify_conn;

static int                  ep = -1;
ngx_uint_t					ngx_use_epoll_rdhup;
static ngx_uint_t			nevents = 512;
static struct epoll_event   *event_list;

#if 0
static void ngx_epoll_notify_handler(ngx_event_t *ev);

static ngx_int_t
ngx_epoll_notify_init(ngx_log_t *log)
{
	struct epoll_event		ee;

	notify_fd = eventfd(0, 0);

	if (notify_fd == -1) {
		ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "eventfd() failed");
		return NGX_ERROR;
	}

	notify_event.handler = ngx_epoll_notify_handler;
	notify_event.log = log;
	notify_event.active = 1;

	notify_conn.fd = notify_fd;
	notify_conn.read = &notify_event;
	notify_conn.log = log;

	ee.events = EPOLLIN | EPOLLET;
	ee.data.ptr = &notify_conn;

	if (epoll_ctl(ep, EPOLL_CTL_ADD, notify_fd, &ee) == -1) {
		ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
					  "epoll_ctl(EPOLL_CTL_ADD, eventfd) failed");
		if (close(notify_fd) == -1) {
			ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
						  "eventfd close() failed");
		}

		return NGX_ERROR;
	}

	return NGX_OK;
}

static void 
ngx_epoll_notify_handler(ngx_event_t *ev)
{
	ssize_t					n;
	uint64_t				count;
	ngx_err_t				err;
	ngx_event_handler_pt	handler;

	if (++ev->index == NGX_MAX_UINT32_VALUE) {
		ev->index = 0;

		n = read(notify_fd, &count, sizeof(uint64_t));

		err = ngx_errno;

		if ((size_t) n != sizeof(uint64_t)) {
			ngx_log_error(NGX_LOG_ALERT, ev->log, err, 
						  "read() eventfd %d failed", notify_fd);
		}
	}

	handler = ev->data;
	handler(ev);
}

#endif

void
ngx_epoll_test_rdhup(ngx_cycle_t *cycle)
{
	int					s[2], events;
	struct epoll_event	ee;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, s) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "socketpair() failed.");
		return;
	}

	ee.events = EPOLLET | EPOLLIN | EPOLLRDHUP;

	if (epoll_ctl(ep, EPOLL_CTL_ADD, s[0], &ee) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "epoll_ctl() failed");
		goto failed;
	}

	if (close(s[1]) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "close() failed.");
		s[1] = -1;
		goto failed;
	}

	s[1] = -1;

	events = epoll_wait(ep, &ee, 1, 5000);

	if (events == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "epoll_wait() failed");
		goto failed;
	}

	if (events) {
		ngx_use_epoll_rdhup = ee.events & EPOLLRDHUP;
	} else {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, NGX_ETIMEDOUT,
					  "epoll_wait() time out");
	}

failed:
	if (s[1] != -1 && close(s[1]) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "close() failed");
	}

	if (close(s[0]) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "close() failed");
	}
}

ngx_int_t
ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags)
{
    int                events;
    uint32_t           revents;
    ngx_int_t          instance, i;
    ngx_uint_t         level;
    ngx_err_t          err;
    ngx_event_t       *rev, *wev;
    ngx_queue_t       *queue;
    ngx_connection_t  *c;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "epoll timer: %M", timer);

    events = epoll_wait(ep, event_list, (int) nevents, timer);

    err = (events == -1) ? ngx_errno : 0;

#if 0
    if (flags & NGX_UPDATE_TIME || ngx_event_timer_alarm) {
        ngx_time_update();
    }
#endif
    
    if (err) {
        if (err == NGX_EINTR) {
#if 0
            if (ngx_event_timer_alarm) {
                ngx_event_timer_alarm = 0;
                return NGX_OK;
            }
#endif

            level = NGX_LOG_INFO;

        } else {
            level = NGX_LOG_ALERT;
        }

        ngx_log_error(level, cycle->log, err, "epoll_wait() failed");
        return NGX_ERROR;
    }

    if (events == 0) {
        if (timer != NGX_TIMER_INFINITE) {
            return NGX_OK;
        }

        ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                "epoll_wait() returned no events without timeout");
        return NGX_ERROR;
    }

    for (i = 0; i < events; i++) {
        c = event_list[i].data.ptr;

        instance = (uintptr_t) c & 1;
        c = (ngx_connection_t *) ((uintptr_t) c & (uintptr_t) ~1);

        rev = c->read;

        if (c->fd == -1 || rev->instance != instance) {
            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "epoll: stale event %p", c);
            continue;
        }

        revents = event_list[i].events;

        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                "epoll: fd:%d ev:%04XD d:%p",
                c->fd, revents, event_list[i].data.ptr);

        if (revents & (EPOLLERR|EPOLLHUP)) {
            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "epoll_wait() error on fd:%d ev:%04XD",
                           c->fd, revents);

            /*
             * if the error events were returned, add EPOLLIN and EPOLLOUT
             * to handle the events at least in one active handler
             */

            revents |= EPOLLIN|EPOLLOUT;
        }

        if ((revents & EPOLLIN) && rev->active) {
            
            if (revents & EPOLLRDHUP) {
                rev->pending_eof = 1;
            }

            rev->available = 1;

            rev->ready =1;

            if (flags & NGX_POST_EVENTS) {
                queue = rev->accept ? &ngx_posted_accept_events
                                    : &ngx_posted_events;

                ngx_post_event(rev, queue);
            } else {
                rev->handler(rev);
            }
        }

        wev = c->write;

        if ((revents & EPOLLOUT) && wev->active) {
            
            if (c->fd == -1 || wev->instance != instance) {

                /*
                 * the stale event from a file descriptor
                 * that was just closed in this iteration
                 */

                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                               "epoll: stale event %p", c);
                continue;
            }

            wev->ready = 1;

            if (flags & NGX_POST_EVENTS) {
                ngx_post_event(wev, &ngx_posted_events);

            } else {
                wev->handler(wev);
            }
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_epoll_init(ngx_cycle_t *cycle)
{
	if (ep == -1) {
		ep = epoll_create(cycle->connection_n / 2);

		if (ep == -1) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
						  "epoll_create() failed");
			return NGX_ERROR;
		}	

#if 0
		if (ngx_epoll_notify_init(cycle->log) != NGX_OK) {
			// NULL
		}
#endif
		ngx_epoll_test_rdhup(cycle);
	}

	if (event_list) {
		ngx_free(event_list);
	}

	event_list = ngx_alloc(sizeof(struct epoll_event) * nevents, cycle->log);

	if (event_list == NULL) {
		return NGX_ERROR;
	}

	//nevents = epcf->events;
    //nevents = 512;
	//ngx_io = ngx_os_io;
	
	ngx_event_flags = NGX_USE_CLEAR_EVENT | NGX_USE_GREEDY_EVENT | NGX_USE_EPOLL_EVENT;

	return NGX_OK;
}

void 
ngx_epoll_done(ngx_cycle_t *cycle)
{
	if (close(ep) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "epoll close() failed");
	}

	ep = -1;
	
	if (close(notify_fd) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, 
					  "eventfd close() failed");
	}

	notify_fd = -1;

	ngx_free(event_list);

	event_list = NULL;
	nevents = 0;
}

ngx_int_t
ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags)
{
	int						op;
	uint32_t				events, prev;
	ngx_connection_t		*c;
	ngx_event_t				*e;
	struct epoll_event		ee;


	c = ev->data;

	events = (uint32_t) event;

    // 读写同操作一个fd, 所以要防止加读\写事件覆盖了之前的读\写事件
	if (event == NGX_READ_EVENT) {
		e = c->write;
		prev = EPOLLOUT;
#if (NGX_READ_EVENT != EPOLLIN|EPOLLRDHUP)
	    events = EPOLLIN | EPOLLRDHUP;	
#endif        
	} else {
		e = c->read;
		prev = EPOLLIN | EPOLLRDHUP;
#if (NGX_WRITE_EVENT != EPOLLOUT)
		events = EPOLLOUT;
#endif
	}

	if (e->active) {
		op = EPOLL_CTL_MOD;
		events |= prev;
	} else {
		op = EPOLL_CTL_ADD;
	}

	ee.events = events | (uint32_t) flags;
	ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);

	if (epoll_ctl(ep, op, c->fd, &ee) == -1) {
		ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
					  "epoll_ctl(%d, %d) failed", op, c->fd);
		return NGX_ERROR;
	}

	ev->active = 1;

	return NGX_OK;
}

ngx_int_t	
ngx_epoll_del_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags)
{
	int					op;
	uint32_t			prev;
	ngx_event_t			*e;
	ngx_connection_t	*c;
	struct epoll_event	ee;

	if (flags & NGX_CLOSE_EVENT) {
		ev->active = 0;
		return NGX_OK;
	}

	c = ev->data;

	if (event == NGX_READ_EVENT) {
		e = c->write;
		prev = EPOLLOUT;

	} else {
		e = c->read;
		prev = EPOLLIN|EPOLLRDHUP;
	}

	if (e->active) {
		op = EPOLL_CTL_MOD;
		ee.events = prev | (uint32_t) flags;
		ee.data.ptr = (void *)((uintptr_t) c | ev->instance);

	} else {
		op = EPOLL_CTL_DEL;
		ee.events = 0;
		ee.data.ptr = NULL;
	}

	if (epoll_ctl(ep, op, c->fd, &ee) == -1) {
		ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
					  "epoll_ctl(%d, %d) failed", op, c->fd);
		return NGX_ERROR;
	}

	ev->active = 0;

	return NGX_OK;
}
