#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

ngx_uint_t			ngx_event_flags;
ngx_uint_t			ngx_use_accept_mutex;
ngx_uint_t          ngx_accept_mutex_held;
ngx_msec_t          ngx_accept_mutex_delay;
ngx_int_t           ngx_accept_disabled;

static ngx_uint_t     ngx_timer_resolution;

void 
ngx_process_events_and_timers(ngx_cycle_t *cycle) 
{
	ngx_uint_t			flags;
	ngx_msec_t			delta, timer;

    if (ngx_timer_resolution) {
        timer = NGX_TIMER_INFINITE;
        flags = 0;
    } else {
        timer = ngx_event_find_timer();
        flags = NGX_UPDATE_TIME;
    }

	if (ngx_use_accept_mutex) {
	   if (ngx_accept_disabled > 0) {
			ngx_accept_disabled--;
	   } else {
#if 0
			if (ngx_trylock_accept_mutex(cycle) = NGX_ERROR) {
				return;
			}

			if (ngx_accept_mutex_held) {
				flags |= NGX_POST_EVENTS;
			}
#endif
	   }
	}

	delta = ngx_current_msec;

	(void) ngx_epoll_process_events(cycle, timer, flags);

	delta = ngx_current_msec - delta;

	ngx_event_process_posted(cycle, &ngx_posted_accept_events);
#if 0
	if (ngx_accept_mutex_held) {
		ngx_shmtx_unlock(&ngx_accept_mutex);
	}
#endif
	if (delta) {
		ngx_event_expire_timers();
	}

	ngx_event_process_posted(cycle, &ngx_posted_events);
}

ngx_int_t	
ngx_event_process_init(ngx_cycle_t *cycle)
{
	ngx_uint_t				i;
	ngx_connection_t		*c, *next; // *old;
	ngx_event_t				*rev, *wev;
	ngx_listening_t			*ls;

    if (NGX_PROCESS_NUM > 2) {
        ngx_use_accept_mutex = 1;
        ngx_accept_mutex_held = 0;
        ngx_accept_mutex_delay = 100;
    } else {
        ngx_use_accept_mutex = 0;
    }

	ngx_queue_init(&ngx_posted_accept_events);
	ngx_queue_init(&ngx_posted_events);

	if (ngx_event_timer_init(cycle->log) == NGX_ERROR) {
	   return NGX_ERROR;
	}

    if (ngx_epoll_init(cycle) != NGX_OK) {
        exit(2);
    }

	cycle->connections = 
		ngx_alloc(sizeof(ngx_connection_t) * cycle->connection_n, cycle->log);
	if (cycle->connections == NULL) {
		return NGX_ERROR;
	}

	c = cycle->connections;

	cycle->read_events = ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n,
									cycle->log);
	if (cycle->read_events == NULL) {
		return NGX_ERROR;
	}

	rev = cycle->read_events;
	for (i = 0; i < cycle->connection_n; i++) {
		rev[i].closed = 1;
		rev[i].instance = 1;
	}

	cycle->write_events = ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n,
									cycle->log);
	if (cycle->write_events == NULL) {
		return NGX_ERROR;
	}

	wev = cycle->write_events;
	for (i = 0; i < cycle->connection_n; i++) {
		wev[i].closed = 1;
	}

	i = cycle->connection_n;
	next = NULL;

	do {
		i--;

		c[i].data = next;
		c[i].read = &cycle->read_events[i];
		c[i].write = &cycle->write_events[i];
		c[i].fd = (ngx_socket_t) -1;

		next = &c[i];
	} while(i);

	cycle->free_connections = next;
	cycle->free_connection_n = cycle->connection_n;

	ls = cycle->listening.elts;
	for (i = 0; i < cycle->listening.nelts; i++) {

		c = ngx_get_connection(ls[i].fd, cycle->log);

		if (c == NULL) {
			return NGX_ERROR;
		}

		c->type = ls[i].type;
		c->log = &ls[i].log;

		c->listening = &ls[i];
		ls[i].connection = c;

		rev = c->read;

		rev->log = c->log;
		rev->accept = 1;

		rev->handler = ngx_event_accept;

		if (ngx_use_accept_mutex) {
            continue;
		}

		if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
			return NGX_ERROR;
		}
	}

	return NGX_OK;
}

ngx_int_t
ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags)
{
    if (!rev->active && !rev->ready) {
        if (ngx_add_event(rev, NGX_READ_EVENT, NGX_CLEAR_EVENT)
                == NGX_ERROR)
        {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}



