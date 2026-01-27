#include <locker.h>


/************************************************************
 * int sem_init(sem_t *sem, int pshared, unsigned int value);
 * int pshared : 共享范围：0表示线程间共享，非0表示进程间共享
 * unsigned int value : 初始化信号量值 
 ************************************************************/
// 初始化化信号量
sem::sem()
{
    if (sem_init(&m_sem, 0, 0) != 0)
    {
        throw std::exception();
    }
}
sem::sem(int sNum){
    if(sem_init(&m_sem,0,sNum) != 0){
        throw std::exception();
    }
}
// 若信号量为0则阻塞，大于0则将信号量-1
bool sem::wait(){
    return sem_wait(&m_sem);
}
// 信号量加1
bool sem::post(){
    return sem_post(&m_sem); 
}



locker::locker(){
    if(pthread_mutex_init(&m_mtx,NULL) != 0){
        throw std::exception();
    }
}
locker::~locker(){
    if(pthread_mutex_destroy(&m_mtx) != 0){
        throw std::exception();
    }
}
void locker::lock(){
    pthread_mutex_lock(&m_mtx);
}
void locker::unlock(){
    pthread_mutex_unlock(&m_mtx);
}
// cond要获取、释放锁时需要用到&mtx
pthread_mutex_t* locker::get(){
    return &m_mtx;
}


cond::cond(){
    if(pthread_cond_init(&m_cv,NULL) != 0){
        throw std::exception();
    }
}
cond::~cond(){
    if(pthread_cond_destroy(&m_cv) != 0){
        throw std::exception();
    }
}
bool cond::wait(pthread_mutex_t* mtx){
    // ****** 不加锁:[ 此处只负责条件变量等待，加锁在外面加，各司其职 ]******
    return pthread_cond_wait(&m_cv,mtx) == 0;
}
bool cond::timeWait(pthread_mutex_t* mtx,struct timespec t){
    return pthread_cond_timedwait(&m_cv,mtx,&t) == 0;
}
bool cond::signal(){
    return pthread_cond_signal(&m_cv) == 0;
}
bool cond::broadcast(){
    return pthread_cond_broadcast(&m_cv) == 0;
}
