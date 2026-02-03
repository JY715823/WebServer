#include "block_queue.h"

// 构造函数
template <class T>
block_queue<T>::block_queue(int maxSize){
    if(maxSize <= 0){
        exit(-1);
    }
    m_maxSize = maxSize;
    m_q.resize(m_maxSize);
    m_size = 0;
    m_front = m_rear = -1;
    m_isStop = false;
}
// 析构函数
template <class T>
block_queue<T>::~block_queue(){
    // 上锁，以免打扰释放vector
    m_mtx.lock();
    m_q.clear();
    m_mtx.unlock();
    // m_mtx和m_cv执行自己的析构函数
}
// 获取当前日志任务数量
template <class T>
int block_queue<T>::size(){
    // 上锁，获取准确的当前个数
    m_mtx.lock();
    int size = m_size;    
    m_mtx.unlock();
    return size;    
}

// 获取队列最大容量
template <class T>
int block_queue<T>::maxSize(){
    m_mtx.lock();
    int maxSize = m_maxSize;
    m_mtx.unlock();
    return maxSize;
}
// 清空队列
template <class T>
void block_queue<T>::clear(){
    m_mtx.lock();
    // m_q.clear();
    // ****** clear问题:[ clear会清空size，此时vector容器访问任何都会越界，所以不要动底层 ]******
    m_size = 0;
    m_front = m_rear = -1;
    m_mtx.unlock();
}
// 判断队列是否满
template <class T>
bool block_queue<T>::isFull(){
    m_mtx.lock();
    if(m_size >= m_maxSize){
        m_mtx.unlock();
        return true;
    }
    m_mtx.unlock();
    return false;
}
// 判断队列是否空
template <class T>
bool block_queue<T>::isEmpty(){
    m_mtx.lock();
    if(m_size <= 0){
        m_mtx.unlock();
        return true;
    }
    m_mtx.unlock();
    return false;
}
// 获取队首元素
template <class T>
bool block_queue<T>::front(T& item){
    m_mtx.lock();
    if(m_size <= 0){
        // ****** 记得解锁再退出:[ 解锁！！！ ]******
        m_mtx.unlock();
        return false;
    }
    item = m_q[(m_front + 1) % m_maxSize];
    m_mtx.unlock();
    return true;
}
// 获取队尾元素
template <class T>
bool block_queue<T>::rear(T& item){
    m_mtx.lock();
    if(m_rear <= 0){
        m_mtx.unlock();
        return false;
    }
    item = m_q[m_rear];
    m_mtx.unlock();
    return true;
}
// 关闭阻塞队列
template <class T>
void block_queue<T>::close(){
    m_mtx.lock();
    m_isStop = true;
    // 唤醒线程
    m_cv.broadcast();
    m_mtx.unlock();
}


// 入队（不阻塞）
template <class T>
bool block_queue<T>::push(T item){
    // 上锁
    m_mtx.lock();
    // 判断队满
    if(m_size >= m_maxSize){
        // 先解锁，再通知消费者，再直接返回false，不等待
        m_mtx.unlock();
        m_cv.broadcast();
        return false;
    }
    // 开始入队
    m_rear = (m_rear + 1) % m_maxSize;
    m_q[m_rear] = item;
    // 更新数量
    m_size++;
    // 解锁
    m_mtx.unlock();
    // 通知消费者
    m_cv.broadcast();
    return true;
}
// 出队（阻塞）
template <class T>
bool block_queue<T>::pop(T& item){
    // 上锁
    m_mtx.lock();
    // 判断队空
    while(m_size <= 0 && !m_isStop){
        m_cv.wait(m_mtx.get());
    }
    // 判断是否终止——队列关闭且没有剩余内容
    if(m_size <= 0 && m_isStop){
        m_mtx.unlock();
        return false;
    }
    // 进行出队
    m_front = (m_front + 1) % m_maxSize;
    item = m_q[m_front];
    // 更新数量
    m_size--;
    // 解锁
    m_mtx.unlock();

    return true;
}
// 出队（超时处理）;
template <class T>
bool block_queue<T>::pop(T& item,int ms_timeout){
    /************************************************************
     *  先拿到当前时间 now
        计算截止时间 deadline = now + ms_timeout
        把 deadline 填到 timespec t 里
        调 pthread_cond_timedwait：要么被 signal 唤醒，要么到点自动醒（超时） 
     ************************************************************/
    // 拿到当前时间
    struct timeval now;
    gettimeofday(&now,nullptr);
    
    struct timespec t;
    t.tv_sec = now.tv_sec + ms_timeout / 1000;
    t.tv_nsec = now.tv_usec * 1000 + (ms_timeout % 1000) * 1000000;
    if (t.tv_nsec >= 1000000000) {
        t.tv_sec += 1;
        t.tv_nsec -= 1000000000;
    }
    m_mtx.lock();
    while(m_size <= 0){
        m_cv.timeWait(m_mtx.get(),t);
    }
    // 判断是否终止——队列关闭且没有剩余内容
    if(m_size <= 0 && m_isStop){
        m_mtx.unlock();
        return false;
    }
    // 进行出队
    m_front = (m_front + 1) % m_maxSize;
    item = m_q[m_front];
    // 更新数量
    m_size--;
    // 解锁
    m_mtx.unlock();

    return true;
}



