#include"locker.h"
/// sem ///
sem::sem(int v){
    if(sem_init(&m_sem, 0, v) != 0){
        throw std::exception();
    }
}

sem::~sem(){
    sem_destroy(&m_sem);
}

bool sem::wait(){
    // 调用成功返回true
    return sem_wait(&m_sem) == 0;
}

bool sem::post(){
    return sem_post(&m_sem) == 0;
}

/// lock ///

locker::locker()
{
    if(pthread_mutex_init(&m_lock, NULL) != 0){
        throw std::exception();
    }
}

locker::~locker()
{
    pthread_mutex_destroy(&m_lock);
}

bool locker::lock(){
    return pthread_mutex_lock(&m_lock) == 0;
}

bool locker::unlock(){
    return pthread_mutex_unlock(&m_lock) == 0;
}

/// con ///
// 条件变量在调用wait时，需要保证原子性，所以需要加锁，调用后再解锁

cond::cond(){
    if(pthread_mutex_init(&m_lock, NULL) != 0){
        throw std::exception();
    }
    if(pthread_cond_init(&m_cond, NULL) != 0){
        pthread_mutex_destroy(&m_lock);
        throw std::exception();
    }
}


cond::~cond(){
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_cond);
}

bool cond::wait(){
    pthread_mutex_lock(&m_lock);
    int ret = pthread_cond_wait(&m_cond, &m_lock);
    pthread_mutex_unlock(&m_lock);
    return ret == 0;
}

bool cond::signal(){
    return pthread_cond_signal(&m_cond) == 0;
}