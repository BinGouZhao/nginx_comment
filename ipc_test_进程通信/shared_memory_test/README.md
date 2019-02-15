#nginx 进程间通信方式--共享内存(nginx/src/os/unix/ngx_shmem.c)

共有三种实现方式

+ 1.mmap 不映射文件
+ 2.mmap 映射 /dev/zero
+ 3.shmget

