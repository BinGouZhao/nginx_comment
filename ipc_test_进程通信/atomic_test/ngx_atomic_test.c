#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SIZE sizeof(int) * 2

/** nginx 原子操作汇编实现, 见/nginx/src/os/unix/ngx_gcc_atomic_x86.h **/
    static inline int
ngx_atomic_fetch_add(int *value, int add)
{
    __asm__ volatile (
	    "lock;"

	    "    xaddl  %0, %1;   "

	    : "+r" (add) : "m" (*value) : "cc", "memory");

    return add;
}

void main() {
    int fd, i;
    pid_t pid;
    void *area;

    if ((fd = open("/dev/zero", O_RDWR)) < 0) {
	printf("run func open fail.\n");
	return;
    }

    if ((area = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, fd, 0)) == MAP_FAILED){
	printf("run func mmap fail.\n");
	return;
    }

    if (close(fd) == -1) {
	printf("run func close fail.\n");
	return;
    }

    /** 父子进程同时对共享内存的数加1, 执行1亿次, 对比使用原子加和不适用原子加的区别 **/
    i = 100000000;
    if ((pid = fork()) < 0) {
	printf("run fork fail.\n");
    } else if (pid > 0) {
	while(i--) {
	    (*(int *)area)++;
	}

	i = 100000000;
	while(i--) {
	    ngx_atomic_fetch_add((int *)(area + sizeof(int)), 1);
	}
	wait(&i);
	printf("p 非原子: %d.\n", *(int *)area);
	printf("p(atomic): %d.\n", *(int *)(area + sizeof(int)));
	printf("两个进程一个加1亿, 应该是两亿.\n");
    } else {
	while(i--) {
	    (*(int *)area)++;
	}

	i = 100000000;
	while(i--) {
	    ngx_atomic_fetch_add((int *)(area + sizeof(int)), 1);
	}
    }
}

