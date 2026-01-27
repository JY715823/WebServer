#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem{
private:
    // 信号量大小
    sem_t m_sem;
public:
    sem();
    sem(int sNum);
    ~sem();
    bool wait();
    bool post();
};

class locker{
private: 
    pthread_mutex_t m_mtx;
public:
    locker();
    ~locker();
    void lock();
    void unlock();
    pthread_mutex_t* get();
};

class cond{
private:
    pthread_cond_t m_cv;
public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t* mtx);
    bool timeWait(pthread_mutex_t* mtx,struct timespec t);
    bool signal();
    bool broadcast();
};