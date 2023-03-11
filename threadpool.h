/*
 * @Author       : WKq
 * @Date         : 2022/09/04
 * @Desc         : 线程池
 */ 

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "locker.h"
#include <exception>
#include <stdexcept>
#include <pthread.h>
#include <list>
#include <functional>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadnums = 8);
    ~ThreadPool();

    template<typename T>
    void AddTask(T&& task);

public:
    static void* work(void* arg);
    void run();

private:
    MyMutex m_mutex;
    MySem m_sem;
    std::list<std::function<void()>> tasks;
    std::vector<pthread_t> m_threads;
    bool isClose;
};

template<typename T>
void ThreadPool::AddTask(T&& task) {
    m_mutex.lock();
    tasks.push_back(std::forward<T>(task));
    m_mutex.unlock();
    m_sem.post();
}

#endif