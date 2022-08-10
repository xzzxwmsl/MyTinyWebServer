//
// Created by xzz on 8/10/22.
//

#ifndef MYTINYWEBSERVER_MUTEXLOCK_H
#define MYTINYWEBSERVER_MUTEXLOCK_H
#include <pthread.h>
#include "nocopyable.h"
class MutexLock : nocopyable
{
    friend class MutexMgr;

private:
    pthread_mutex_t _mutex;
    void lock()
    {
        pthread_mutex_lock(&_mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

public:
    MutexLock()
    {
        pthread_mutex_init(&_mutex, nullptr);
    }
    ~MutexLock()
    {
        pthread_mutex_lock(&_mutex);
    }
};

class MutexMgr : nocopyable
{
private:
    MutexLock &mutex;

public:
    explicit MutexMgr(MutexLock &_mutex) : mutex(_mutex) { mutex.lock(); }
    ~MutexMgr() { mutex.unlock(); }
};

#endif // MYTINYWEBSERVER_MUTEXLOCK_H
