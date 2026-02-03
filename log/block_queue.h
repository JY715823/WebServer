/************************************************************
 * 阻塞队列，用环形数组实现，用于异步日志 
 * 主线程将日志内容push进队列，写日志（到磁盘）线程从队列取出日志内容进行IO操作
 * 防止IO操作阻塞主线程
 ************************************************************/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include "../lock/locker.h"

// 使用模板，以后可以扩展日志内容的类型
template <class T> 
class block_queue{
public:
    // 构造函数
    block_queue(int maxSize = 1000);
    // 析构函数
    ~block_queue();
    // 获取当前日志任务数量
    int size();
    // 获取队列最大容量
    int maxSize();
    // 清空队列
    void clear();
    // 判断队列是否满
    bool isFull();
    // 判断队列是否空
    bool isEmpty();
    // 获取队首元素
    bool front(T& item);
    // 获取队尾元素
    bool rear(T& item);
    // 关闭阻塞队列
    void close();

    // 入队（不阻塞）
    bool push(T item);
    // 出队（阻塞）
    bool pop(T& item);
    // 出队（超时处理）;
    bool pop(T& item,int ms_timeout);


private: 
    vector<T> m_q; // 环形数组
    int m_size; // 当前元素数量
    int m_maxSize; // 数组最大容量
    int m_front,m_rear; // 队头，队尾
    locker m_mtx; // 互斥锁保护临界区
    cond m_cv; // 条件变量（用于唤醒消费者线程）
    // ****** 条件变量:[ 只当队列为空时消费者阻塞，队列满的时候push失败直接返回false，采用同步日志 ]******
    // ****** 新增成员:[ 阻塞队列结束标志 ]******
    bool m_isStop;
};

#include "block_queue.tpp"

#endif