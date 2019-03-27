#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_channel.h>
#include <ngx_websocket.h>
#include <ngx_redis_message.h>

static void ngx_master_process_exit(ngx_cycle_t *cycle);
static void ngx_start_worker_processes(ngx_cycle_t *cycle, ngx_int_t n,
    ngx_int_t type);
static void ngx_worker_process_cycle(ngx_cycle_t *cycle, void *data);
static void ngx_worker_process_init(ngx_cycle_t *cycle, ngx_int_t worker);

static void ngx_start_websocket_message_process(ngx_cycle_t *cycle, ngx_int_t type);
static void ngx_websocket_message_process_cycle(ngx_cycle_t *cycle, void *data);
static void ngx_websocket_message_process_init(ngx_cycle_t *cycle);
static void ngx_process_websocket_messages(ngx_cycle_t *cycle);
static void ngx_channel_handler(ngx_event_t *ev);
static void ngx_signal_worker_processes(ngx_cycle_t *cycle, int signo);
static void ngx_pass_open_channel(ngx_cycle_t *cycle, ngx_channel_t *ch);
static void ngx_websocket_new_message(ngx_cycle_t *cycle);

ngx_uint_t      ngx_process;
ngx_uint_t      ngx_worker;
ngx_pid_t       ngx_pid;
ngx_pid_t       ngx_parent;

ngx_uint_t      ngx_process;
ngx_uint_t      ngx_daemonized;

sig_atomic_t  ngx_reap;
sig_atomic_t  ngx_sigio;
sig_atomic_t  ngx_sigalrm;
sig_atomic_t  ngx_terminate;
sig_atomic_t  ngx_quit;
sig_atomic_t  ngx_debug_quit;
ngx_uint_t    ngx_exiting;
sig_atomic_t  ngx_reconfigure;
sig_atomic_t  ngx_reopen;

sig_atomic_t  ngx_change_binary;
ngx_pid_t     ngx_new_binary;
ngx_uint_t    ngx_inherited;
ngx_uint_t    ngx_daemonized;

sig_atomic_t  ngx_noaccept;
ngx_uint_t    ngx_noaccepting;
ngx_uint_t    ngx_restart;

static u_char  master_process[] = "master process";

void 
ngx_master_process_cycle(ngx_cycle_t *cycle)
{
    char              *title;
    u_char            *p;
    size_t             size;
    ngx_int_t          i;
    ngx_uint_t         n, live, sigio;
    ngx_listening_t   *ls;
    sigset_t           set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, ngx_signal_value(NGX_RECONFIGURE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_REOPEN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_NOACCEPT_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_TERMINATE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_CHANGEBIN_SIGNAL));

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "sigprocmask() failed");
    }

    sigemptyset(&set);

    size = sizeof(master_process);

    for (i = 0; i < ngx_argc; i++) {
        size += ngx_strlen(ngx_argv[i]) + 1;
    }

    title = ngx_pnalloc(cycle->pool, size);
    if (title == NULL) {
        exit(2);
    }

    p = ngx_cpymem(title, master_process, sizeof(master_process) - 1);
    for (i = 0; i < ngx_argc; i++) {
        *p++ = ' ';
        p = ngx_cpystrn(p, (u_char *) ngx_argv[i], size);
    }

    ngx_setproctitle(title);

    ngx_start_worker_processes(cycle, NGX_PROCESS_NUM, NGX_PROCESS_RESPAWN);
    ngx_start_websocket_message_process(cycle, NGX_PROCESS_DETACHED);

    sigio = 0;
    //live = 1;
    live = 0;

    for ( ;; ) {

        sigsuspend(&set);

        ngx_time_update();

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, 
                       "wake up, sigio %i", sigio);

        if (ngx_terminate) {
            ngx_signal_worker_processes(cycle, 
                                        ngx_signal_value(NGX_TERMINATE_SIGNAL));
        }

        if (ngx_quit) {
            ngx_signal_worker_processes(cycle,
                    ngx_signal_value(NGX_SHUTDOWN_SIGNAL));

            ls = cycle->listening.elts;
            for (n = 0; n < cycle->listening.nelts; n++) {
                if (ngx_close_socket(ls[n].fd) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno,
                            ngx_close_socket_n " %V failed",
                            &ls[n].addr_text);
                }
            }
            cycle->listening.nelts = 0;

            continue;
        }

        if (!live && (ngx_terminate || ngx_quit)) {
            ngx_master_process_exit(cycle);
        }
    }
}

static void 
ngx_master_process_exit(ngx_cycle_t *cyle)
{
    exit(0);
}

static void
ngx_start_websocket_message_process(ngx_cycle_t *cycle, ngx_int_t type)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "start websocket message process");

    ngx_spawn_process(cycle, ngx_websocket_message_process_cycle, NULL, "websocket message process", type); 

}

static void 
ngx_start_worker_processes(ngx_cycle_t *cycle, ngx_int_t n, ngx_int_t type)
{
    ngx_int_t           i;
    ngx_channel_t       ch;

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "start worker processes");

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_OPEN_CHANNEL;

    for (i = 0; i < n; i++) {
        ngx_spawn_process(cycle, ngx_worker_process_cycle,
                          (void *) (intptr_t) i, "worker process", type);

        ch.pid = ngx_processes[ngx_process_slot].pid;
        ch.slot = ngx_process_slot;
        ch.fd = ngx_processes[ngx_process_slot].channel[0];

        ngx_pass_open_channel(cycle, &ch);
    }
}

static void
ngx_pass_open_channel(ngx_cycle_t *cycle, ngx_channel_t *ch)
{
    ngx_int_t  i;

    for (i = 0; i < ngx_last_process; i++) {

        if (i == ngx_process_slot
            || ngx_processes[i].pid == -1
            || ngx_processes[i].channel[0] == -1)
        {
            continue;
        }

        ngx_log_debug6(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                      "pass channel s:%i pid:%P fd:%d to s:%i pid:%P fd:%d",
                      ch->slot, ch->pid, ch->fd,
                      i, ngx_processes[i].pid,
                      ngx_processes[i].channel[0]);

        /* TODO: NGX_AGAIN */

        ngx_write_channel(ngx_processes[i].channel[0],
                          ch, sizeof(ngx_channel_t), cycle->log);
    }
}

static void
ngx_websocket_message_process_cycle(ngx_cycle_t *cycle, void *data)
{
    ngx_process = NGX_PROCESS_WORKER;

    ngx_websocket_message_process_init(cycle);

    ngx_setproctitle("websocket message process");

    for ( ;; ) {
        if (ngx_exiting) {
            exit(2);
        }

        ngx_process_websocket_messages(cycle);

        if (ngx_terminate || ngx_quit) {
            exit(2);
        }
    }
}

static void
ngx_process_websocket_messages(ngx_cycle_t *cycle)
{
	ssize_t					    n;
    ngx_err_t                   err;
    ngx_int_t                   ret;
    ngx_uint_t                  length;
	ngx_socket_t			    fd;
	ngx_redis_connection_t	    *rc;
    ngx_websocket_message_t     *m;

	rc = cycle->redis;

	n = ngx_redis_read_message(rc);

    if (n == 0) {
		ngx_log_error(NGX_LOG_ALERT, rc->log, ngx_errno,
					  "redis server has gone away.");
	}

	if (n == 0 || n == NGX_ERROR) {

		fd = rc->fd;
		rc->fd = (ngx_socket_t) -1;

		if (fd != -1) {
			if (ngx_close_socket(fd) == -1) {

				err = ngx_socket_errno;

				ngx_log_error(NGX_LOG_CRIT, rc->log, err, ngx_close_socket_n " %d failed", fd);
			}
		}

		if (ngx_redis_init_connection(cycle) == NGX_ERROR) {
			exit(2);
		}

		if (ngx_redis_subscribe(cycle->redis) == NGX_ERROR) {
			exit(2);
		}
	}

	ret = 0;

    for ( ;; ) {
        if (ret == NGX_AGAIN) {
            if (rc->buffer->pos == rc->buffer->last) {
				if (rc->parse_done) {
					// 数据区重置
					rc->buffer->pos = rc->buffer->start;
					rc->buffer->last = rc->buffer->start;
				}
			}

			break;
        }

		if (rc->parse_done) {
			if (rc->buffer->end - rc->buffer->pos < 
					NGX_WEBSOCKET_MAX_MESSAGE_LENGTH - (ngx_int_t) rc->channel_message_prefix_n) 
			{
				ngx_memcpy(rc->parse_start, rc->buffer->start, rc->buffer->last - rc->parse_start); 
			}
		}

        ret = ngx_process_parse_message(rc, rc->buffer);

        if (ret == NGX_OK) {
            if (rc->error) {
                ngx_log_error(NGX_LOG_ALERT, rc->log, ngx_errno,
                        "redis server has a error: %s", rc->error_start);
                exit(2);
            }

            length = rc->message_end - rc->message_start;

            m = &ngx_messages[ngx_message_index];
            m->message_id = ngx_message_id;
            m->channel_id = 1;

            m->message_length = ngx_websocket_frame_encode(m->message, rc->message_start, NGX_WS_OPCODE_TEXT, 1, length);

            ngx_message_id++;
            ngx_message_index++;

            if (ngx_message_index == NGX_WEBSOCKET_MESSAGE_N) {
                ngx_message_index = 0;
            }

			rc->parse_done = 1;
			rc->parse_start = rc->buffer->pos;
			ngx_websocket_new_message(cycle);

            continue;
        }

        if (ret == NGX_AGAIN) {
            continue;
        }

        ngx_log_error(NGX_LOG_ALERT, rc->log, ngx_errno,
                      "redis server has a error: %d", ret);
        exit(2);
    }

    return;
}

static void 
ngx_worker_process_cycle(ngx_cycle_t *cycle, void *data)
{
    ngx_int_t worker = (intptr_t) data;

    ngx_process = NGX_PROCESS_WORKER;
    ngx_worker = worker;

    ngx_worker_process_init(cycle, worker);

    ngx_setproctitle("worker process");

    for ( ;; ) {

        if (ngx_exiting) {
            exit(2);
        }

        ngx_process_events_and_timers(cycle);

        if (ngx_terminate || ngx_quit) {
            exit(2);
        }
    }
}

static void
ngx_websocket_message_process_init(ngx_cycle_t *cycle)
{
    sigset_t            set;
    ngx_time_t          *tp;
    
    sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                "sigprocmask() failed");
    }

    tp = ngx_timeofday();
    srandom(((unsigned) ngx_pid << 16) ^ tp->sec ^ tp->msec);

	if (ngx_redis_init_connection(cycle) == NGX_ERROR) {
		exit(2);
	}

	if (ngx_redis_subscribe(cycle->redis) == NGX_ERROR) {
		exit(2);
	}
}

static void
ngx_channel_handler(ngx_event_t *ev)
{
    ngx_int_t          n;
    ngx_channel_t      ch;
    ngx_connection_t  *c;

    if (ev->timedout) {
        ev->timedout = 0;
        return;
    }

    c = ev->data;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ev->log, 0, "channel handler");

    for ( ;; ) {

        n = ngx_read_channel(c->fd, &ch, sizeof(ngx_channel_t), ev->log);

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0, "channel: %i", n);

        if (n == NGX_ERROR) {

            if (ngx_event_flags & NGX_USE_EPOLL_EVENT) {
                ngx_del_conn(c, 0);
            }

            ngx_close_connection(c);
            return;
        }

        if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {
            if (ngx_add_event(ev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return;
            }
        }

        if (n == NGX_AGAIN) {
            return;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0,
                       "channel command: %ui", ch.command);

        switch (ch.command) {

        case NGX_CMD_QUIT:
            ngx_quit = 1;
            break;

        case NGX_CMD_TERMINATE:
            ngx_terminate = 1;
            break;

        case NGX_CMD_REOPEN:
            ngx_reopen = 1;
            break;

        case NGX_CMD_OPEN_CHANNEL:

            ngx_log_debug3(NGX_LOG_DEBUG_CORE, ev->log, 0,
                           "get channel s:%i pid:%P fd:%d",
                           ch.slot, ch.pid, ch.fd);

            ngx_processes[ch.slot].pid = ch.pid;
            ngx_processes[ch.slot].channel[0] = ch.fd;
            break;

        case NGX_CMD_CLOSE_CHANNEL:

            ngx_log_debug4(NGX_LOG_DEBUG_CORE, ev->log, 0,
                           "close channel s:%i pid:%P our:%P fd:%d",
                           ch.slot, ch.pid, ngx_processes[ch.slot].pid,
                           ngx_processes[ch.slot].channel[0]);

            if (close(ngx_processes[ch.slot].channel[0]) == -1) {
                ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                              "close() channel failed");
            }

            ngx_processes[ch.slot].channel[0] = -1;
            break;

        case NGX_CMD_NEW_MESSAGE:
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "channel recv new message");
            break;
        }
    }
}

static void
ngx_worker_process_init(ngx_cycle_t *cycle, ngx_int_t worker)
{
    sigset_t          set;
    ngx_int_t         n;
    ngx_time_t        *tp;
    
    sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                "sigprocmask() failed");
    }

    tp = ngx_timeofday();
    srandom(((unsigned) ngx_pid << 16) ^ tp->sec ^ tp->msec);

    ngx_event_process_init(cycle);

    for (n = 0; n < ngx_last_process; n++) {

        if (ngx_processes[n].pid == -1) {
            continue;
        }

        if (n == ngx_process_slot) {
            continue;
        }

        if (ngx_processes[n].channel[1] == -1) {
            continue;
        }

        if (close(ngx_processes[n].channel[1]) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "close() channel failed");
        }
    }

    if (close(ngx_processes[ngx_process_slot].channel[0]) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "close() channel failed");
    }

    if (ngx_add_channel_event(cycle, ngx_channel, NGX_READ_EVENT,
                ngx_channel_handler)
            == NGX_ERROR)
    {
        /* fatal */
        exit(2);
    }
}

static void
ngx_websocket_new_message(ngx_cycle_t *cycle)
{
    ngx_int_t      i;
    ngx_channel_t  ch;

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_NEW_MESSAGE;
    ch.fd = -1;

    for (i = 0; i < ngx_last_process; i++) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "child: %i %P e:%d t:%d d:%d r:%d j:%d",
                       i,
                       ngx_processes[i].pid,
                       ngx_processes[i].exiting,
                       ngx_processes[i].exited,
                       ngx_processes[i].detached,
                       ngx_processes[i].respawn,
                       ngx_processes[i].just_spawn);

        if (ngx_processes[i].detached || ngx_processes[i].pid == -1) {
            continue;
        }

        if (ngx_processes[i].just_spawn) {
            ngx_processes[i].just_spawn = 0;
            continue;
        }

        if (ngx_processes[i].exiting) {
            continue;
        }

        if (ch.command) {
            if (ngx_write_channel(ngx_processes[i].channel[0],
                                  &ch, sizeof(ngx_channel_t), cycle->log)
                == NGX_OK)
            {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, 0, 
                              "websocket message process send channel(new message) success.");
                continue;
            } else {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, 
                        "websocket message process send channel(new message) failed.");
                continue;
            }
        }
    }
}

static void
ngx_signal_worker_processes(ngx_cycle_t *cycle, int signo)
{
    ngx_int_t      i;
    ngx_err_t      err;
    ngx_channel_t  ch;

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    switch (signo) {

    case ngx_signal_value(NGX_SHUTDOWN_SIGNAL):
        ch.command = NGX_CMD_QUIT;
        break;

    case ngx_signal_value(NGX_TERMINATE_SIGNAL):
        ch.command = NGX_CMD_TERMINATE;
        break;

    case ngx_signal_value(NGX_REOPEN_SIGNAL):
        ch.command = NGX_CMD_REOPEN;
        break;

    default:
        ch.command = 0;
    }

    ch.fd = -1;

    for (i = 0; i < ngx_last_process; i++) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "child: %i %P e:%d t:%d d:%d r:%d j:%d",
                       i,
                       ngx_processes[i].pid,
                       ngx_processes[i].exiting,
                       ngx_processes[i].exited,
                       ngx_processes[i].detached,
                       ngx_processes[i].respawn,
                       ngx_processes[i].just_spawn);

        if (ngx_processes[i].message && ngx_processes[i].pid != -1) {
            if (kill(ngx_processes[i].pid, signo) == -1) {
                err = ngx_errno;
                ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                        "kill(%P, %d) failed", ngx_processes[i].pid, signo);

                if (err == NGX_ESRCH) {
                    ngx_processes[i].exited = 1;
                    ngx_processes[i].exiting = 0;
                    ngx_reap = 1;
                }

                continue;
            }

            continue;
        }

        if (ngx_processes[i].detached || ngx_processes[i].pid == -1) {
            continue;
        }

        if (ngx_processes[i].just_spawn) {
            ngx_processes[i].just_spawn = 0;
            continue;
        }

        if (ngx_processes[i].exiting
            && signo == ngx_signal_value(NGX_SHUTDOWN_SIGNAL))
        {
            continue;
        }

        if (ch.command) {
            if (ngx_write_channel(ngx_processes[i].channel[0],
                                  &ch, sizeof(ngx_channel_t), cycle->log)
                == NGX_OK)
            {
                if (signo != ngx_signal_value(NGX_REOPEN_SIGNAL)) {
                    ngx_processes[i].exiting = 1;
                }

                continue;
            }
        }

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                       "kill (%P, %d)", ngx_processes[i].pid, signo);

        if (kill(ngx_processes[i].pid, signo) == -1) {
            err = ngx_errno;
            ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                          "kill(%P, %d) failed", ngx_processes[i].pid, signo);

            if (err == NGX_ESRCH) {
                ngx_processes[i].exited = 1;
                ngx_processes[i].exiting = 0;
                ngx_reap = 1;
            }

            continue;
        }

        if (signo != ngx_signal_value(NGX_REOPEN_SIGNAL)) {
            ngx_processes[i].exiting = 1;
        }
    }
}
