#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// 非阻塞方式
int
ngx_trylock_fd(int fd)
{
	struct flock f1;

	memset(&f1, '0', sizeof(struct flock));
	f1.l_type = F_WRLCK;
	f1.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &f1) == -1) {
	   return errno;
	}

	return 0;
}

// 阻塞
int 
ngx_lock_fd(int fd) 
{
	struct flock f1;

	memset(&f1, '0', sizeof(struct flock));
	f1.l_type = F_WRLCK;
	f1.l_whence = SEEK_SET;

	if(fcntl(fd, F_SETLKW, &f1) == -1) {
	   return errno;
	}
	
	return 0;
}

int 
ngx_unlock_fd(int fd)
{
	struct flock f1;
	
	memset(&f1, '0', sizeof(struct flock));
	f1.l_type = F_UNLCK;
	f1.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &f1) == -1) {
		return errno;
	}

	return;
}

void main() {
	int fd;

	if ((fd = open("/dev/null", O_RDWR)) == -1) {
		printf("run func open() failed.\n");
		return;
	}

	switch (fork()) {
		case -1:
			printf("run func fork() failed.\n");
			return;

		case 0:
			if (ngx_lock_fd(fd) != 0) {
				printf("run func ngx_lock_fd() failed.\n");
				return;
			} else {
				printf("get lock(in child).\n");
			}

			sleep(10);

			if (ngx_unlock_fd(fd) != 0) {
				printf("unlock failed, errno: %d.\n", errno);
			} else {
				printf("unlock success(in child).\n");
			}
			break;

		default:
			sleep(5);

			if (ngx_trylock_fd(fd) != 0) {
				printf("try get lock failed(noblocking)(in parent), return errno: %d.\n", errno);
			} else {
				printf("get lock success(noblocking)(in parent).\n");
			}

			printf("try get lock use block method(in parent).\n");

			if (ngx_lock_fd(fd) != 0) {
				printf("try get lock failed(blocking)(in parent), return errno: $d.\n", errno);
			} else {
				printf("get lock success(blocking)(in parent).\n");
			}
			break;
	}

	close(fd);
	return;
}
