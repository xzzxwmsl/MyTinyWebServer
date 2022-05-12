#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H
#include <pthread.h>
#include <fcntl.h>
#include "../locker/locker.h"
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <string.h>
#include <assert.h>
// 子进程类
class process
{
public:
    pid_t pid;
    // 管道，父子进程间通信
    int pipefd[2];

    explicit process(int p = -1) : pid(p){};
    ~process();
};

template <typename T>
class processpool
{
private:
    // 构造函数设定为私有
    processpool(int listenfd, int process_number = 8);
    // 单例模式，全局静态变量
    // static processpool<T> *instance = nullptr;
    // 进程池最大子进程数量
    static const int MAX_PROCESS_NUMBER = 16;
    // 每个子进程最多能处理的客户数量
    static const int USER_PER_PROCESS = 65535;
    // epoll最多能处理的事件数
    static const int MAX_EVENT_NUMBER = 10000;
    // 进程池中的进程总数
    int m_process_number;
    // 子进程在池中的序号，从0开始
    int m_idx;
    // 每个进程都有一个epoll内核事件表，用m_epollfd标识
    int m_epollfd;
    // 监听socket
    int m_listenfd;
    // 子进程通过m_stop决定运行状态
    bool m_stop;
    // 保存子进程描述信息
    process *m_sub_process;

    void setup_sig_pipe();
    void run_parent();
    void run_child();

public:
    // 单例模式,magic static,程序只能创建一个进程池实例
    // 原书中可能会出现竞态条件
    static processpool<T> &getInstance(int listenfd, int process_number = 8);
    virtual ~processpool() = default;
    void run();
};
// 信号管道
static int sig_pipefd[2];

static int setnonblocking(int fd)
{
    int oldopt = fcntl(fd, F_GETFL);
    int newopt = oldopt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newopt);
    return oldopt;
}

static void addfd(int epollfd, int fd)
{
    // 在epoll中注册事件
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd)
{
    // 中epollfd注册的epoll内核事件表中删除fd注册的事件
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig)
{
    // 会将信号发给管道sig_pipefd[1]
    int save_error = errno;
    int msg = sig;
    send(sig_pipefd[1], (char *)(&msg), 1, 0);
    errno = save_error;
}

static void addsig(int sig, void(handler)(int), bool restart = true)
{
    // 添加信号
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

template <typename T>
processpool<T> &processpool<T>::getInstance(int listenfd, int process_number)
{
    static processpool<T> instance(listenfd, process_number);
    return instance;
}

template <typename T>
processpool<T>::processpool(int listenfd, int process_number) : m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)
{
    assert(m_process_number <= MAX_PROCESS_NUMBER && m_process_number > 0);
    // 新建子进程记录体
    m_sub_process = new process[m_process_number];
    assert(m_sub_process);

    // 创建子进程
    for (int i = 0; i < process_number; i++)
    {
        int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_sub_process[i].pipefd);
        assert(ret == 0);
        m_sub_process[i].pid = fork();
        assert(m_sub_process[i].pid >= 0);
        if (m_sub_process[i].pid > 0)
        {
            // 在父进程
            close(m_sub_process[i].pipefd[1]);
            // 父进程，继续创建子进程
            continue;
        }
        {
            // 子进程
            printf("processpool->construct : create child process %d, pid%d\n", i, m_sub_process[i].pid);
            close(m_sub_process[i].pipefd[0]);
            m_idx = i;
            break;
        }
    }
}

template <typename T>
void processpool<T>::run()
{
    if (m_idx == -1)
    {
        // m_idx为-1的是父进程
        run_parent();
    }
    else
    {
        run_child();
    }
}

template <typename T>
void processpool<T>::setup_sig_pipe()
{
    // 创建epoll事件监听表和信号管道
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    // 监听sig_pipefd[0]
    addfd(m_epollfd, sig_pipefd[0]);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

template <typename T>
void processpool<T>::run_parent()
{
    // 主进程
    // 创建管道sig_pipefd，注册监听事件
    setup_sig_pipe();
    // 注册监听m_listenfd
    addfd(m_epollfd, m_listenfd);

    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;
    // epoll就绪事件数
    int number = 0;
    int ret = -1;
    while (!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            // 就绪事件
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd)
            {
                // 新的连接到来，采用轮转方式将其分配给子进程处理
                int i = sub_process_counter;
                do
                {
                    // 找到可用的
                    if (m_sub_process[i].pid != -1)
                    {
                        break;
                    }
                    i = (i + 1) % m_process_number;
                } while (i != sub_process_counter);
                // 如果找到的子进程不存在
                if (m_sub_process[i].pid == -1)
                {
                    m_stop = true;
                    break;
                }
                sub_process_counter = (i + 1) % m_process_number;
                // 通过双向管道向子进程发送新连接通知
                send(m_sub_process[i].pipefd[0], (char *)(&new_conn), sizeof(new_conn), 0);
                printf("processpool->parent : send request to child %d", i);
            }
            else if (sockfd == sig_pipefd[0] && events[i].events & EPOLLIN)
            {
                // 处理父进程收到的信号
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0)
                {
                    continue;
                }
                else
                {
                    for (int i = 0; i < ret; i++)
                    {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        {
                            // 子进程退出,那么关闭其对应的管道并将pid标记为-1
                            pid_t c_pid;
                            int c_stat;
                            while ((c_pid = waitpid(-1, &c_stat, WNOHANG)) > 0)
                            {
                                // 如果所有的子进程都结束了，那么退出
                                m_stop = true;
                                for (int j = 0; j < m_process_number; j++)
                                {
                                    if (m_sub_process[j].pid == c_pid)
                                    {
                                        // close it
                                        printf("processpool->parent : child %d join\n", j);
                                        close(m_sub_process[j].pipefd[0]);
                                        m_sub_process[j].pid = -1;
                                    }
                                    if (m_sub_process[j].pid != -1)
                                    {
                                        // 有一个没结束，那么不必退出
                                        m_stop = false;
                                    }
                                }
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            // 终止所有子进程
                            printf("kill all subprocess\n");
                            for (int j = 0; j < m_process_number; j++)
                            {
                                if (m_sub_process[j].pid != -1)
                                {
                                    kill(m_sub_process[j].pid, SIGTERM);
                                }
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }
                    }
                }
            }
            else
            {
                continue;
            }
        }
    }

    // 终止
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(m_epollfd);
}

template <typename T>
void processpool<T>::run_child()
{
    setup_sig_pipe();
    // 与父进程通信的pipefd
    int pipefd = m_sub_process[m_idx].pipefd[1];
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    // 服务对象
    T *users = new T[USER_PER_PROCESS];
    assert(users);

    int number = 0;
    int ret = -1;

    while (!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            printf("processpool->child %d, epoll failure\n", m_idx);
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == pipefd && events[i].events & EPOLLIN)
            {
                // 接收到了来自父进程的管道新数据
                int client = 0;
                ret = recv(pipefd, (char *)&client, sizeof(client), 0);
                if ((ret < 0 && errno != EAGAIN) || ret == 0)
                {
                    continue;
                }
                else
                {
                    // 接受新连接
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int connfd = accept(m_listenfd, (sockaddr *)&client_addr, &client_len);
                    if (connfd < 0)
                    {
                        printf("processpool->child %d, errno is %d\n", m_idx, errno);
                        continue;
                    }
                    // 与新连接建立了socket通信，监听这个端口
                    addfd(m_epollfd, connfd);
                    // 模板类必须实现init方法，以初始化一个客户连接。我们通过connfd直接索引一个对象
                    users[connfd].init(m_epollfd, connfd, client_addr);
                }
            }
            else if (sockfd == sig_pipefd[0] && events[i].events & EPOLLIN)
            {
                // 子进程接收到了信号
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0)
                {
                    continue;
                }
                else
                {
                    for (int j = 0; j < ret; j++)
                    {
                        switch (signals[j])
                        {
                        case SIGCHLD:
                        {
                            pid_t c_pid;
                            int c_stat;
                            // 非阻塞模式
                            while (c_pid = waitpid(-1, &c_stat, WNOHANG) > 0)
                            {
                                continue;
                            }
                            break;
                        }
                        case SIGINT:
                        case SIGTERM:
                        {
                            m_stop = true;
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                // 客户请求到来
                // 客户模板必须实现process功能
                users[sockfd].process();
            }
            else
            {
                continue;
            }
        }
    }
    delete[] users;
    users = nullptr;
    close(m_epollfd);
}


#endif
