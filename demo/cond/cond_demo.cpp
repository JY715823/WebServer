#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <queue> // 用于生产者消费者模型的队列

// 初始化互斥锁和条件变量
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; 
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static std::queue<int> q; 


// 消费者线程
void* consumer(void* arg){
    // 获取互斥锁
    pthread_mutex_lock(&mtx);

    // 循环检查队列是否有元素可以消费，没有元素就需要一直等待
    // ******[ 必须用while，防止虚假唤醒，比如在条件变量满足后线程被唤醒，
    // 但是此时另一个线程抢先消费了资源，此时while继续检查发现资源没有了又继续等待才合理]******
    while(q.empty()){
        pthread_cond_wait(&cv,&mtx);
    }
    // 访问临界区，从队列里取出元素
    int x = q.front();
    q.pop();
    // 释放互斥锁
    pthread_mutex_unlock(&mtx);

    // 处理资源
    std::cout << x << std::endl;

    // ******[ 显式返回NULL ]******
    return NULL;
}


int main(){
    // 启动消费者线程（1个）
    pthread_t tid;
    if(pthread_create(&tid,NULL,&consumer,NULL) != 0){
        // 错误处理
    }

    sleep(1);

    // 主线程作为生产者向队列里放数据
    // 获取互斥锁
    pthread_mutex_lock(&mtx);
    q.push(92);
    // 释放互斥锁
    pthread_mutex_unlock(&mtx);
    /************************************************************
     * 要在释放锁之后唤醒消费者线程，不然如果还没释放就唤醒消费者线程
     * 消费者线程会干等生产者释放锁 
     ************************************************************/
    pthread_cond_signal(&cv);

    pthread_join(tid,NULL);

    return 0;
}