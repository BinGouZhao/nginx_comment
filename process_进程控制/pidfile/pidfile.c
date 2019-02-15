#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define u_char unsigned char
#define NGX_MAX_PATH 4096

int
ngx_create_pidfile(char *name) {
	int fd;
	char *p;
	size_t len;
	u_char pid[22];

	if (name == NULL) {
		return -1;
	}

	p = malloc(NGX_MAX_PATH);
	if (p == NULL) {
		return -1;
	}

	if (name[0] != '/') {
		if ((getcwd(p, NGX_MAX_PATH)) == NULL) {
			free(p);
			return -1;
		}

		len = strlen(p);
		p[len] = '/';
		strcpy(p + len + 1, name);
	} else {
		strcpy(p, name);
	}

	fd = open((const char *)p, O_RDWR | O_CREAT, 0600);
	free(p);

	if (fd == -1) {
		return -1;
	}

	len = snprintf(pid, 22, "%d\n", getpid());
	write(fd, pid, len);
	if (close(fd) == -1) {
		return -1;
	}
	return 0;
}

void main() {
	if (ngx_create_pidfile("nginx.pid") == -1) {
		printf("create pid file failed.\n");
	}

	printf("pid: %d.\n", getpid());
}

