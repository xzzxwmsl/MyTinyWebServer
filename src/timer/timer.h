#ifndef TIMER_H
#define TIMER_H
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <list>
#include <fcntl.h>
#include<time.h>
#include <vector>
#include <unordered_map>
// 前向声明
class util_timer;
// 用户数据结构
struct client_data
{
    // 客户端socket地址
    sockaddr_in address;
    // socket文件描述符
    int sockfd;
    // 定时器
    util_timer *timer;
};

class util_timer
{
public:
    util_timer() = default;
    util_timer(time_t exp, client_data *cd) : expire(exp), user_data(cd) {}
    ~util_timer();
    // 回调函数
    void (*cb_func)(client_data *);
    // 超时时间
    time_t expire;
    // 连接资源
    client_data *user_data;
};

// 基于队列实现的定时器
class sort_timer_lst
{
private:
    // 保存定时器
    std::list<util_timer *> lst;
    // 保存定时器对应的迭代器位置，方便O(1)查找
    std::unordered_map<util_timer *, std::list<util_timer *>::iterator> cache;

public:
    sort_timer_lst();
    ~sort_timer_lst();
    void add_timer(util_timer *);
    void adjust_timer(util_timer *);
    void del_timer(std::list<util_timer*>::iterator);
    void del_timer(util_timer *);
    void trick();
    inline size_t getSize() const { return lst.size(); }
};

#endif