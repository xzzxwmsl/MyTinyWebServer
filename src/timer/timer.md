# Timer
改进书上的项目，使用优先队列构建定时器

## 概念
* 非活跃：客户端与服务端建立连接，长时间不交换数据，一直占用服务器端的文件描述符等资源
* 定时事件：固定时间触发的代码，由该段代码处理事件
* 定时器：利用结构体或其他形式，将某个事件集合封装起来。本项目封装一个检测非活跃连接的定时器
* 定时器容器：利用某种容器类数据结构，将多个定时器组装起来，便于对定时器统一管理

## Linux定时方法
* socket选项：SO_RECVTIMEO 和 SO_SNDTIMEO ： 分别用来设置socket接收数据超时时间和发送数据超时时间，所以用于与socket数据收发有关的系统调用，如send,recv,sendmsg,recvmsg,accept,connect等。
* SIGALRM信号 ： 利用alarm函数周期性触发SIGALRM信号，信号处理函数利用管道通知主循环，主循环接收到信号后对定时器容器里的所有定时器进行处理，释放非活跃连接
* IO复用系统的超时参数

## 基础API

**sigaction结构体**

```cpp
struct sigaction
{
    // 指向信号处理函数
    void (*sa_handler)(int);

    // 也是信号处理函数，参数更多，可以获得关于信号更详细的信息
	void (*sa_sigaction) (int, siginfo_t *, void *);

    // 信号掩码，指定需要被屏蔽的信号
    __sigset_t sa_mask;

    // 指定信号处理行为
    int sa_flags;

    // 一般没用
    void (*sa_restorer) (void);
};
```

```cpp
#include<signal.h>

// 功能是检查或修改与指定信号相关联的处理动作（可同时两种操作）
// signum : 要捕获的信号类型 act : 对信号设置新的处理方式 oldact : 输出信号原先的处理方式
int sigactiom(int signum, const struct sigaction *act, struct sigaction *oldact);

// 将参数set信号集初始化，然后将所有的信号加入到此信号集里
int sigfill(sigset_t *set);

// 设置信号传送闹钟，即用来设置信号SIGALRM在经过参数seconds秒数后发送给目前的进程。如果未设置信号SIGALRM的处理函数，那么alarm()默认处理终止进程.
unsigned int alarm(unsigned int seconds);

#include<sys/socket.h>
// sv[2]表示套接字柄对，可以双向通信
int socketpair(int domain, int type, int protocol, int sv[2]);
```