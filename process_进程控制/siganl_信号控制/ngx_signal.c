#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

#define ngx_signal_helper(n)     SIG##n
#define ngx_signal_value(n)      ngx_signal_helper(n)

#define NGX_SHUTDOWN_SIGNAL      QUIT
#define NGX_TERMINATE_SIGNAL     TERM

#define NGX_PROCESS_MASTER 1
#define NGX_PROCESS_WORKER 3

void ngx_signal_handler(int sig);
void ngx_process_get_status();

int ngx_process;

typedef struct {
	int  signo;
	char *signame;
	char *name;
	void (*handler)(int signo);
} ngx_signal_t;

typedef int ngx_err_t;

sig_atomic_t ngx_quit;
sig_atomic_t ngx_terminate;

ngx_signal_t signals[] = {
	{ ngx_signal_value(NGX_SHUTDOWN_SIGNAL),
	  "SIGQUIT",
  	  "quit",
      ngx_signal_handler },

	{ SIGTERM, "SIGTERM", "stop", ngx_signal_handler },

	{ SIGINT, "SIGINT", "", ngx_signal_handler },

	{ SIGCHLD, "SIGCHLD", "", ngx_signal_handler }
};

int
ngx_init_signals() {
	ngx_signal_t		*sig;
	struct sigaction	sa;

	for (sig = signals; sig->signo != 0; sig++) {
		memset(&sa, '0', sizeof(struct sigaction));
		sa.sa_handler = sig->handler;
		sigemptyset(&sa.sa_mask);
		if (sigaction(sig->signo, &sa, NULL) == -1) {
			printf("run func sigaction(%s) failed.\n", sig->name);
			return -1;
		}
	}

	return 0;
}

void
ngx_signal_handler(int signo)
{
	char 			*action;
	ngx_err_t		err;
	ngx_signal_t 	*sig;

	err = errno;

	for (sig = signals; sig->signo != 0; sig++) {
		if (sig->signo == signo) {
			break;
		}
	}

	action = "";

	switch (ngx_process) {
		case NGX_PROCESS_MASTER:
			switch (signo) {
				case ngx_signal_value(NGX_SHUTDOWN_SIGNAL):
					ngx_quit = 1;
					action = "shutting down(IN MASTER)";
					break;

				case ngx_signal_value(NGX_TERMINATE_SIGNAL):
				case SIGINT:
					ngx_terminate = 1;
					action = "exiting(IN MASTER)";
					break;

				case SIGCHLD:
					action = "get child exit status(IN MASTER)";
					break;
			}

			break;
		case NGX_PROCESS_WORKER:
			switch (signo) {
				case ngx_signal_value(NGX_SHUTDOWN_SIGNAL):
					ngx_quit = 1;
					action = "shutting down(IN WORKER)";
					break;

				case ngx_signal_value(NGX_TERMINATE_SIGNAL):
				case SIGINT:
					ngx_terminate = 1;
					action = "exiting(IN WORKER)";
					break;
			}

			break;
	}

	printf("catch signal %d, to do: %s.\n", signo, action);

	if (signo == SIGCHLD) {
		ngx_process_get_status();
	}

	errno = err;
}

void 
ngx_process_get_status() {
	int			status, pid;
	ngx_err_t	err;

	for ( ;; ) {
		pid = waitpid(-1, &status, WNOHANG);

		if (pid == 0) {
			return;
		}

		if (pid == -1) {
			err = errno;

			if (err == EINTR) {
				continue;
			}

			if (err == ECHILD) {
				printf("run func waitpid() failed.\n");
				return;
			}

			printf("run func waitpid() failed.\n");
			return;
		}

		if (WTERMSIG(status)) {
			printf("process(%d) exited on signal %d.\n", pid, WTERMSIG(status));
			return;
		} else {
			printf("process(%d) exited with code  %d.\n", pid, WEXITSTATUS(status));
			return;
		}
	}
}

int main() {
	if (ngx_init_signals() == -1) {
		printf("init signals failed.\n");
		return -1;
	}
	
	switch (fork()) {
		case -1:
			printf("run func fork() failed.\n");
			return -1;
		case 0:
			ngx_process = NGX_PROCESS_WORKER;
			break;
		default:
			ngx_process = NGX_PROCESS_MASTER;
			break;
	}

	for ( ;; ) {
		if (ngx_quit) {
			sleep(3);
			return 0;
		}

		if (ngx_terminate) {
			exit(2);
		}
	}
}
