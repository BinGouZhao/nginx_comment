#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SIZE sizeof(int)

int run = 1;

static void
handle_int(int sig) {
    run = 0;
}

static int 
update(int *ptr) {
    return ((*ptr)++);
}

void main() {
    int fd;
    pid_t pid;
    void *area;  /** 共享内存 **/

    signal(SIGINT, handle_int);

    if ((fd = open("/dev/zero", O_RDWR)) < 0) {
	printf("run func open fail.\n");
	return;
    }

    if ((area = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
	printf("run func mmap fail.\n");
	return;
    }

    if (close(fd) == -1) {
	printf("run func close fail.\n");
	return;
    }

    /** 子进程与父进程访问共享内存( 修改变量不是原子操作 ） **/
    if ((pid = fork()) < 0) {
	printf("run func fork fail.\n");
	return;
    } else if (pid > 0) {
	while(run) {
	    sleep(2);
	    printf("p: %d\n", *(int *)area);
	    update((int *)area);
	}
    } else {
	while(run) {
	    sleep(4);
	    printf("c: %d\n", *(int *)area);
	    update((int *)area);
	}
    }
}

