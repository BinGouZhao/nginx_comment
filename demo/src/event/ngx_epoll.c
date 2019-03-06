#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

static int					notify_fd = -1;
ngx_event_t					notify_event;
static ngx_connection_t		notify_conn;

ngx_uint_t					ngx_use_epoll_rdhup;
static ngx_uint_t			nevents;
static struct pollfd		*event_list;

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

static void
ngx_epoll_test_rdhup(ngx_cycle_t *cycle)
{
	int					s[2], events;
	struct epoll_event	ee;

	if (sockpair(AF_UNIX, SOCK_STREAM, 0, s) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "sockpair() failed.");
		return;
	}

	ee.events = EPOLLET | EPOLLIN | EPOLLRDHUP;

	if (epoll_ctl(ep, EPOLL_CTL_ADD, s[0], &ee) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "epoll_ctl() failed");
		goto failed;
	}

	if (close(s[i]) == -1) {
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

static ngx_int_t
ngx_epoll_init(ngx_cycle_t *cycle)
{
	if (ep == -1) {
		ep = epoll_create(cycle->connection_n / 2);

		if (ep == -1) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
						  "epoll_create() failed");
			return NGX_ERROR;
		}	

		if (ngx_epoll_notify_init(cycle->log) != NGX_OK) {
			//
		}

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
	//ngx_io = ngx_os_io;
	
	ngx_event_flags = NGX_USE_CLEAR_EVENT | NGX_USE_GREEDY_EVENT | NGX_USE_EPOLL_EVENT;

	return NGX_OK;
}

static void 
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

static ngx_int_t
ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags)
{
	int						op;
	uint32_t				events, prev;
	ngx_connection_t		*c;
	ngx_event_t				*e;
	struct epoll_event		ee;


	c = ev->data;

	events = (uint32_t) event;

	if (evnet == NGX_READ_EVENT) {
		e = c->write;
		prev = EPOLLOUT;
	    events = EPOLLIN | EPOLLRDHUP;	
	} else {
		e = c->read;
		prev = EPOLLIN | EPOLLRDHUP;
		events = EPOLLOUT;
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

static ngx_int_t	
ngx_epoll_del_event(ngx_event *ev, ngx_int_t event, ngx_uint_t flags)
{
	int					op;
	ngx_event			*e;
	ngx_connection_t	*c;
	struct epoll_fd		ee;
	uint32_t			prev;

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





