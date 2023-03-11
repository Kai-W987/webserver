#include "threadpool.h"

ThreadPool::ThreadPool(size_t threadnums) :m_threads(threadnums), isClose(false){
    if (threadnums <= 0) {
        throw std::runtime_error("threadnums is zero");
    }

    for (int ii = 0; ii < threadnums; ii++) {
        if (pthread_create(&m_threads[ii], NULL, work, this) != 0) {
            printf ("ii\n");
            throw std::runtime_error("pthread_create wrong");
        }

        if (pthread_detach(m_threads[ii]) != 0) {
            throw std::runtime_error("pthread_detach wrong");
        }
    }
}

ThreadPool::~ThreadPool() {
    if (isClose == false) {
        isClose = true;
    }
}

void* ThreadPool::work(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool *>(arg);
    pool->run();
    return pool;
}

void ThreadPool::run() {
    while (!isClose) {
        m_sem.wait();
        m_mutex.lock();
        
        if (tasks.empty()) {
            m_mutex.unlock();
            continue;
        }

        auto task = std::move(tasks.front());
        tasks.pop_front();
        m_mutex.unlock();

        task();
    }
}