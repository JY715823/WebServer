#include <iostream>
#include <pthread.h>
#include <queue>
#include <unistd.h>

class boundedBlockingQueue{
private:
    int cap; // 队列容量上限
    std::queue<int> q; // 队列
    pthread_mutex_t mtx; // 互斥锁保护临界区
    pthread_cond_t cv_notfull; // 队列满的条件变量(阻塞唤醒生产者)
    pthread_cond_t cv_notempty; // 队列空的条件变量（阻塞唤醒消费者）
    /************************************************************
     * 如果只用一个 cond，那么：
        生产者和消费者都在同一个 cond 上等待
        你 signal 一次，可能唤醒“错误角色”（比如队列满时唤醒生产者，醒来发现还是满，又睡回去）
        会造成更多无效唤醒，性能更差，逻辑也更容易写乱 
     ************************************************************/
    /************************************************************
     * 什么时候一个条件变量也挺合适？
            线程数很少，性能无所谓
            你愿意接受更多无效唤醒
            或者你设计成：用一个 cv，但更倾向用 broadcast 简化逻辑（牺牲性能换简单）
       典型“一个 cond 的正确模板”是这样的：
            push 后 signal/broadcast
            pop 后 signal/broadcast 
     ************************************************************/
public:
    boundedBlockingQueue(int cap) : cap(cap){
        pthread_mutex_init(&mtx,NULL);
        pthread_cond_init(&cv_notfull,NULL);
        pthread_cond_init(&cv_notempty,NULL);
    }
    ~boundedBlockingQueue(){
        pthread_mutex_destroy(&mtx);
        pthread_cond_destroy(&cv_notfull);
        pthread_cond_destroy(&cv_notempty);
    }

    void push(int x){
        // ****** 阻塞push:[ 队列满了就进入阻塞 ]******
        // 先为共享队列加锁
        pthread_mutex_lock(&mtx);
        // 等待条件变量满足
        while(q.size() >= cap){
            // 容量满了进入阻塞
            pthread_cond_wait(&cv_notfull,&mtx);
        }
        // 入队操作
        q.push(x);
        // 解锁
        pthread_mutex_unlock(&mtx);
        // 唤醒消费者
        pthread_cond_signal(&cv_notempty);
    }

    int pop(){
        // ****** 阻塞式出队:[ 队列空时进入阻塞 ]******
        // 给临界区加锁
        pthread_mutex_lock(&mtx);
        // 等待条件变量
        while(q.empty()){
            pthread_cond_wait(&cv_notempty,&mtx);
        }
        // 出队
        int x = q.front();
        q.pop();
        // 解锁
        pthread_mutex_unlock(&mtx);
        // 唤醒生产者
        pthread_cond_signal(&cv_notfull);

        // 返回出队的元素
        return x;
    }
};


// ****** 测试:[ 两个生产者，一个消费者，让生产效率大于消费效率，观察阻塞 ]******
struct Args{
    boundedBlockingQueue* q;
    int id;
};

// 生产者线程
void* producer(void* args){
    Args* a = static_cast<Args*>(args);
    for(int i = 0;i < 10;i++){
        int val = a->id * 100 + i;
        a->q->push(val);
        std::cout << "producer[" << a->id << "] push " << val << std::endl;
        usleep(30 * 1000); 
    }

    // 显式返回空指针
    return nullptr;
}

// 消费者线程
void* comsumer(void* args){
    auto* a = (Args*)args;
    // ****** 资源数量要对齐:[ 生产者和消费者生产/消费的资源应该相等，如果生产者生产多了那生产者就会一直阻塞 ]******
    for(int i = 0;i < 20;i++){
        int val = a->q->pop();
        std::cout << "comsumer[" << a->id << "] pop " << val << std::endl;
        usleep(80 * 1000);
    }

    return nullptr;
}


int main(){
    boundedBlockingQueue q(3);
    pthread_t t1,t2,t3;
    Args a1{&q,1},a2{&q,2},ac{&q,0};

    pthread_create(&t1,NULL,&producer,&a1);
    pthread_create(&t2,NULL,&producer,&a2);
    pthread_create(&t3,NULL,&comsumer,&ac);

    pthread_join(t1,nullptr);
    pthread_join(t2,nullptr);
    pthread_join(t3,nullptr);

    return 0;
}




