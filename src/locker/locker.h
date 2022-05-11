#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <semaphore.h>
// 封装信号量的类
class sem
{
private:
    sem_t m_sem;
public:
    sem(int v = 0);
    ~sem();
    bool wait();
    bool post();
};

class locker
{
private:
    pthread_mutex_t m_lock;
public:
    locker(/* args */);
    ~locker();
    bool lock();
    bool unlock();
};

class cond
{
private:
    pthread_cond_t m_cond;
    pthread_mutex_t m_lock;
public:
    cond();
    ~cond();
    bool wait();
    bool signal();
};
#endif