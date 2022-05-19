#include <stdio.h>
#include "src/threadpool/threadpool.h"
#include "src/locker/locker.h"
#include "src/httpconn/http_conn.h"
#include "src/timer/timer.h"
#include <assert.h>

const int MAX_FD = 65535;
const int MAX_EVENT_NUMBER = 10000;

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int, int);

void addsig(int sig, void (*handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void show_error(int connfd, const char *info)
{
    printf("%s\n", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main()
{
    int port = 9002;
    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // 忽略SIGPIPE信号
    addsig(SIGPIPE, SIG_IGN);
    threadpool<http_conn> *pool = nullptr;
    try
    {
        pool = new threadpool<http_conn>();
    }
    catch (...)
    {
        return 1;
    }

    // 为每个可能的客户连接分配一个http_conn对象
    http_conn *users = new http_conn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);
    // 立即关闭该连接，通过发送RST分组(而不是用正常的FIN|ACK|FIN|ACK四个分组)
    // 来关闭该连接。至于发送缓冲区中如果有未发送完的数据，则丢弃。
    // 主动关闭一方的TCP状态则跳过TIMEWAIT，直接进入CLOSED。
    struct linger temp = {1, 0};
    // 设置函数close()关闭TCP连接时的行为。
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));

    int ret = bind(listenfd, (struct sockaddr *)(&address), sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    while (true)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EAGAIN)
        {
            printf("EPOLL FAILURE\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                printf("youxinlianjie\n");
                // 新连接
                sockaddr_in client_address;
                socklen_t client_len = sizeof(client_address);
                int connfd = accept(sockfd, (sockaddr *)(&client_address), &client_len);
                if (connfd < 0)
                {
                    continue;
                }
                if (http_conn::m_user_count > MAX_FD)
                {
                    show_error(connfd, "Internal Server Busy\n");
                    continue;
                }
                users[connfd].init(connfd, client_address, (char*)"/root");
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }
            else if (events[i].events & EPOLLIN)
            {
                // 在主进程读取数据
                if (users[sockfd].read())
                {   
                    pool->append(users + sockfd);
                }
                else
                {
                    users[sockfd].close_conn();
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                printf("接收缓冲区好了\n");
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
            else
            {
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete[] pool;
    return 0;
}