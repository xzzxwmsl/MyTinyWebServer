#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "../locker/locker.h"
#include <list>
#include <stdio.h>
template <typename T>
class threadpool
{
private:
    // 工作线程运行的函数，从工作队列中取出任务并执行
    static void *worker(void *argc);
    void run();

    // 线程池中线程数
    int m_thread_number;
    // 请求队列中允许的最大请求数
    int m_max_requests;
    // 线程池的数组
    pthread_t *m_threads;
    // 请求队列与锁
    std::list<T *> m_workqueue;
    locker m_queuelock;
    // 是否有任务需要处理
    sem m_queuestat;
    bool m_stop;

public:
    explicit threadpool(int thread_num = 8, int max_requests = 10000);
    virtual ~threadpool();
    bool append(T *request);
};

template <typename T>
threadpool<T>::threadpool(int thread_num, int max_requests) : m_thread_number(thread_num), m_max_requests(max_requests), m_stop(false), m_threads(nullptr)
{
    if (thread_num <= 0 || max_requests <= 0)
    {
        printf("threadpool->construct, should greater than 0\n");
        throw std::exception();
    }
    // 创建存储线程id的数组索引
    m_threads = new pthread_t[m_thread_number];
    for (int i = 0; i < m_thread_number; i++)
    {
        printf("threadpool->construct, create %d thread\n", i);
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            printf("threadpool->construct, create thread %d failure\n", i);
            delete[] m_threads;
            throw std::exception();
        }
        // 设置为脱离线程，不会产生僵尸线程
        // 分离一个线程：通知操作系统该线程结束时，可以直接回收资源
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    m_stop = true;
    delete[] m_threads;
}

template <typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelock.lock();
    if (m_workqueue.size() > m_max_requests)
    {
        m_queuelock.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    // 告知队列长度加一
    m_queuestat.post();
    m_queuelock.unlock();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *argc)
{
    threadpool *tp = reinterpret_cast<threadpool *>(argc);
    tp->run();
    return tp;
}

template <typename T>
void threadpool<T>::run()
{
    while (!m_stop)
    {
        // 等待有可用的
        m_queuestat.wait();
        // 抢占操作队列的机会
        m_queuelock.lock();
        if (m_workqueue.empty())
        {
            m_queuelock.unlock();
            continue;
        }
        T *task = m_workqueue.front();
        m_workqueue.pop_front();
        // 释放队列锁
        m_queuelock.unlock();
        if (task == nullptr)
            continue;
        task->process();
    }
}
#endif