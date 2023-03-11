/*
 * @Author       : WKq
 * @Date         : 2022/09/04
 */ 

#ifndef _LOCKER_H
#define _LOCKER_H

#include <stdexcept>
#include <exception>
#include <pthread.h>
#include <semaphore.h>

class MyMutex {
public:
    MyMutex();
    ~MyMutex();
    bool lock();
    bool unlock();
    pthread_mutex_t *get();

private:
    pthread_mutex_t m_mutex;
};

class MySem {
public:
    MySem();
    explicit MySem(int num);
    ~MySem();
    bool wait();
    bool timewait(const struct timespec abs_timeout);
    bool post();

private:
    sem_t m_sem;
};

class MyCond {
public:
    MyCond();
    ~MyCond();
    bool wait(pthread_mutex_t* mutex);
    bool timewait(pthread_mutex_t* mutex, const struct timespec abs_timeout);
    bool signal();  //至少解锁一个
    bool broadcast();   //解锁全部

private:
    pthread_cond_t m_cond;
};

#endif