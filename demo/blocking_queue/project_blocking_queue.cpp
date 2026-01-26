#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <vector>


class projectBlockingQueue{
private:
    std::vector<int> q; // 用环形数组来做队列
    int front; int rear; // 队头队尾
    int size; // 队列元素数
    int cap; // 队列最大容量
    pthread_mutex_t mtx; // 互斥锁保护临界区
    pthread_cond_t cv_notempty; // 条件变量，用来唤醒消费者

public:
    projectBlockingQueue(int cap) : q(cap),cap(cap),size(0),front(-1),rear(-1){
        pthread_mutex_init(&mtx,nullptr);
        pthread_cond_init(&cv_notempty,nullptr);
    }
    ~projectBlockingQueue(){
        pthread_mutex_destroy(&mtx);
        pthread_cond_destroy(&cv_notempty);
    }

    bool push(int x){
        // ****** 非阻塞式入队:[ 队满的时候不阻塞，直接返回false ]******
        // 加锁
        pthread_mutex_lock(&mtx);
        // 检查条件
        if(size >= cap){
            // 直接返回
            pthread_mutex_unlock(&mtx); // 先解锁！！！
            return false;
        }
        // 进行入队
        rear = (rear + 1) % cap;
        q[rear] = x;
        size++;
        // 解锁
        pthread_mutex_unlock(&mtx);

        // ****** 修复:[ 必须唤醒消费者 ]******
        pthread_cond_signal(&cv_notempty);

        return true;
    }

    bool pop(int& x){
        // ****** 阻塞式出队:[ 队空的时候进行阻塞，等待生产者生产 ]******
        // 加锁
        pthread_mutex_lock(&mtx);
        // 检查条件变量
        while(size <= 0){
            pthread_cond_wait(&cv_notempty,&mtx);
        }
        // 进行出队
        front = (front + 1) % cap;
        x = q[front];
        size--;
        // 解锁
        pthread_mutex_unlock(&mtx);

        return true;
    }
};


// ****** 测试:[ 一个生产者疯狂push，一个消费者慢慢pop ]******
struct Args{
    projectBlockingQueue* q;
};

// 生产者线程
void* producer(void* p) {
    auto* a = (Args*)p;
    int dropped = 0;
    for (int i = 0; i < 200; ++i) {
        if (!a->q->push(i)) {
            ++dropped; // 满了会丢
        }
        usleep(2 * 1000);
    }
    printf("[producer] dropped=%d\n", dropped);
    return nullptr;
}

// 消费者线程
void* consumer(void* p) {
    auto* a = (Args*)p;
    for (int i = 0; i < 200; ++i) {
        int x;
        a->q->pop(x);
        // 故意慢一点，制造“队列满导致丢弃”
        usleep(10 * 1000);
    }
    return nullptr;
}



int main(){
    projectBlockingQueue q(8);
    pthread_t tp, tc;
    Args a{&q};
    pthread_create(&tp, nullptr, producer, &a);
    pthread_create(&tc, nullptr, consumer, &a);
    pthread_join(tp, nullptr);
    pthread_join(tc, nullptr);
    return 0;
}