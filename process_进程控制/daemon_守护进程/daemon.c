#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

pid_t ngx_pid;

int 
ngx_daemon() {
	int fd;

	switch (fork()) {
		case -1:
			printf("run func fork() failed.\n");
			return -1;
		
		case 0:
			break;
		
		default:
			exit(0);
	}

	ngx_pid = getpid();

	if (setsid() == -1) {
		printf("run func setsid() failed.\n");
		return -1;
	}

	fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		printf("open /dev/null failed.\n");
		return -1;
	}

	if (dup2(fd, STDIN_FILENO) == -1) {
		printf("dup2(STDIN) failed.\n");
		return -1;
	}

	if (dup2(fd, STDOUT_FILENO) == -1) {
		printf("dup2(STDOUT) failed.\n");
		return -1;
	}

	if (fd > STDERR_FILENO) {
		if (close(fd) == -1) {
			printf("run func close() failed.\n");
			return -1;
		}
	}

	return 0;
}

int main() {
	if (ngx_daemon() == -1) {
		return 1;
	}

	printf("test the stdout, pid: %d.\n", ngx_pid);
	sleep(30);
}





