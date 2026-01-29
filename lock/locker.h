#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem{
public:
    sem();
    sem(int sNum);
    ~sem();
    bool wait();
    bool post();
private:
    // 信号量大小
    sem_t m_sem;
};

class locker{
public:
    locker();
    ~locker();
    void lock();
    void unlock();
    pthread_mutex_t* get();
private: 
    pthread_mutex_t m_mtx;
};

class cond{
public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t* mtx);
    bool timeWait(pthread_mutex_t* mtx,struct timespec t);
    bool signal();
    bool broadcast();
private:
    pthread_cond_t m_cv;
};