#include "locker.h"

MyMutex::MyMutex() {
    if (pthread_mutex_init(&m_mutex, NULL) != 0) {
        throw std::runtime_error("pthread_mutex_init wrong");
    }
}

MyMutex::~MyMutex() {
    pthread_mutex_destroy(&m_mutex);
}

bool MyMutex::lock() {
    return pthread_mutex_lock(&m_mutex) == 0;
}

bool MyMutex::unlock() {
    return pthread_mutex_unlock(&m_mutex) == 0;
}

pthread_mutex_t* MyMutex::get() {
    return &m_mutex;
}

MySem::MySem() {
    if (sem_init(&m_sem, 0, 0) != 0) {
        throw std::runtime_error("sem_init wrong");
    }
}

MySem::MySem(int num) {
    if (sem_init(&m_sem, 0, num) != 0) {
        throw std::runtime_error("sem_init(int num) wrong");
    }
}

MySem::~MySem() {
    sem_destroy(&m_sem);
}

bool MySem::wait() {
    return sem_wait(&m_sem) == 0;
}

bool MySem::timewait(const struct timespec abs_timeout) {
    return sem_timedwait(&m_sem, &abs_timeout) == 0;
}

bool MySem::post() {
    return sem_post(&m_sem);
}

MyCond::MyCond() {
    if (pthread_cond_init(&m_cond, NULL) != 0) {
        throw std::runtime_error("pthread_cond_init wrong");
    }
}

MyCond::~MyCond() {
    pthread_cond_destroy(&m_cond);
}

bool MyCond::wait(pthread_mutex_t* mutex) {
    return pthread_cond_wait(&m_cond, mutex) == 0;
}

bool MyCond::timewait(pthread_mutex_t* mutex, const struct timespec abs_timeout) {
    return pthread_cond_timedwait(&m_cond, mutex, &abs_timeout) == 0;
}

bool MyCond::signal() {
    return pthread_cond_signal(&m_cond) == 0;
}

bool MyCond::broadcast() {
    return pthread_cond_broadcast(&m_cond) == 0;
}