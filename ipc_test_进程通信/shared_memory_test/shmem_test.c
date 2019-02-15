#include <stdio.h>
#include <sys/shm.h>
#include <signal.h>

int run = 1;

static void 
handle_int(int sig) {
    run = 0;
}

void main() {
    int shmid;
    pid_t pid;
    void *shmptr;

    signal(SIGINT, handle_int);

    if ((shmid = shmget(IPC_PRIVATE, sizeof(int), 600)) < 0) {
	printf("run func shmget fail.\n");
	return;
    }

    /** 运行时报错, errno = 13, 权限错误, 转root运行通过 **/
    if ((shmptr = shmat(shmid, NULL, 0)) == (void *)-1) {
	printf("run func shmat fail.\n");
	return;
    }

    if ((pid = fork()) < 0) {
	printf("run fork fail.\n");
	return;
    } else if (pid > 0) {
	while(run) {
	    sleep(2);
	    printf("p: %d.\n", *(int *)shmptr); 
	    (*(int *)shmptr)++;
	}
	if (shmctl(shmid, IPC_RMID, 0) < 0) {
	    printf("run func shmctl fail.\n");
	    return;
	}
	shmdt(shmptr);
	waitpid(pid, NULL, 0);
    } else {
	while(run) {
	    sleep(4);
	    printf("c: %d.\n", *(int *)shmptr);
	    (*(int *)shmptr)++;
	}
    }
}

