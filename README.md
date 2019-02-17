# nginx 源码学习及功能测试

想要学习nginx源码, 但苦于nginx代码数量庞大、结构复杂, 故将nginx按功能划分为数个模块，分段研究。 

#### 2019年2月4日

进程通信之共享内存

nginx 原子操作实现

#### 2019年2月15日

进程控制(守护进程实现、进程名修改、创建进程pid文件)

信号控制(控制nginx的停止，热重启, 重读配置等)

#### 2019年2月17日

文件锁实现(fcntl)

ngx_queue 双向链表实现

