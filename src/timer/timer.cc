#include "timer.h"

sort_timer_lst::sort_timer_lst() {}

sort_timer_lst::~sort_timer_lst()
{
    // 删除定时器内的所有事件
    while (!lst.empty())
    {
        del_timer(lst.front());
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    // 只能增加一个副本
    if (timer == nullptr || cache.count(timer))
    {
        return;
    }
    for (auto it = lst.begin(); it != lst.end(); it++)
    {
        if (timer->expire <= (*it)->expire)
        {
            auto t = lst.insert(it, timer);
            cache[timer] = t;
            return;
        }
    }
    // 插入到尾巴上
    lst.push_back(timer);
    cache[timer] = --lst.end();
}

void sort_timer_lst::del_timer(util_timer *timer)
{
    if (timer == nullptr)
    {
        return;
    }
    if (cache.count(timer))
    {
        lst.erase(cache[timer]);
        cache.erase(timer);
        delete timer;
    }
}

void sort_timer_lst::del_timer(std::list<util_timer*>::iterator it)
{
    util_timer* temp = *it;
    cache.erase(temp);
    lst.erase(it);
    delete temp;
}

// 仅考虑超时时间延长的情况
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    auto nextIter = cache[timer];
    nextIter++;

    lst.erase(cache[timer]);
    cache.erase(timer);

    for (auto it = nextIter; it != lst.end(); it++)
    {
        if (timer->expire <= (*it)->expire)
        {
            auto t = lst.insert(it, timer);
            cache[timer] = t;
            return;
        }
    }
    // 插入到尾巴上
    lst.push_back(timer);
    cache[timer] = --lst.end();
}

void sort_timer_lst::trick()
{
    auto it = lst.begin();
    time_t cur_time = time(NULL);
    while (it != lst.end())
    {
        if (cur_time < (*it)->expire)
        {
            break;
        }
        // 执行任务
        (*it)->cb_func((*it)->user_data);
        auto temp = it;
        ++it;
        // 删除资源
        del_timer(temp);
    }
}