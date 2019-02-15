#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ngx_strlen(s) strlen((const char *) s)
#define u_char unsigned char

extern char **environ;
static char *ngx_os_argv_last;
static char **ngx_os_argv;

u_char *
ngx_cpystrn(u_char *dst, u_char *src, size_t n)
{
	if (n == 0) {
		return dst;
	}

	while (--n) {
		*dst = *src;
		
		if (*dst == '\0') {
			return dst;
		}

		dst++;
		src++;
	}

	*dst = '\0';

	return dst;
}

int
ngx_init_setproctitle() {
	unsigned char *p;
	size_t size;
	unsigned int i;

	size = 0;

	for (i = 0; environ[i]; i++) {
		size += ngx_strlen(environ[i]) + 1;
	}

	p = malloc(size);
	if (p == NULL) {
		return -1;
	}
	ngx_os_argv_last = ngx_os_argv[0];

	for (i = 0; ngx_os_argv[i]; i++) {
		if (ngx_os_argv_last == ngx_os_argv[i]) {
			ngx_os_argv_last = ngx_os_argv[i] + ngx_strlen(ngx_os_argv[i]) + 1;
		}
	}

	for (i = 0; environ[i]; i++) {
		if (ngx_os_argv_last == environ[i]) {
			size = ngx_strlen(environ[i]) + 1;
			ngx_os_argv_last = environ[i] + size;

			ngx_cpystrn(p, (unsigned char *)environ[i], size);
			environ[i] = (char *) p;
			p += size;
		}
	}

	ngx_os_argv_last--;

	return 1;
}

void 
ngx_setproctitle(char *title)
{
	u_char *p;

	ngx_os_argv[1] = NULL;

	p = ngx_cpystrn((u_char *)ngx_os_argv[0], (u_char *)"nginx: ", 
					ngx_os_argv_last - ngx_os_argv[0]);

	p = ngx_cpystrn(p, (u_char *) title, ngx_os_argv_last - (char *) p);

	if (ngx_os_argv_last - (char *) p) {
		memset(p, '\0', ngx_os_argv_last - (char *)p);
	}
}

void save_argv(int argc, char *const *argv)
{
    ngx_os_argv = (char **) argv;
}

void main(int argc, char **argv) {
	save_argv(argc, argv);
	ngx_init_setproctitle();
	ngx_setproctitle("master");
	printf("the value of argv[0]: %s.\n", argv[0]);
	sleep(30);
}
