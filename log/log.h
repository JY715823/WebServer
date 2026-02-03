#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

// ****** Log类采用单例模式:[ 全局用唯一一个Log实例化的对象 ]******
class Log{
public:
    // ****** 懒汉单例模式:[ 第一次使用到Log的时候再对唯一的实例进行初始化 ]******
    static Log* get_instance();
    // ****** 启动异步日志工作线程:[ 作为桥接函数,具体见static函数的作用 ]******
    static void* start_log_thread(void* args);
    // ****** 对日志系统进行初始化:[ 可通过外部传参来控制相关参数,没有传参就使用默认参数 ]******
    bool init(const char* file_name,bool close_log,int log_buf_size = 8192,
                int split_lines = 1000,int max_queue_size = 0);
    // ****** 日志系统核心:[ 将日志通过同步/异步的方式写入磁盘持久化 ]******
    void write_log(int level,const char* format,...);
    // 刷新日志系统缓冲区到磁盘里
    void flush();


private:
    Log();
    ~Log();

    // 异步写日志任务
    void async_write_log();
    // 获取当前日历时间
    void get_time(struct timeval& tv,struct tm& now);

    char dir_name[128]; // 目录路径名
    char log_name[128]; // 日志文件名
    //char* m_buf; // 日志缓冲区
    int m_split_lines; // 日志最大行数
    int m_log_buf_size; // 日志缓冲区大小
    long long m_count; // 日志行数记录
    int m_today; // 按天分日志,记录今天是哪一天
    FILE* m_fp; // 打开log文件的文件指针
    bool m_close_log; // 关闭日志标志位
    // 异步日志相关
    pthread_t m_tid; // 异步日志线程号;
    block_queue<string>* m_q; // 阻塞队列
    bool m_is_async; // 异步标志位
    locker m_mtx; // 保护临界区
};


// 宏函数实现对外提供的统一函数
#define LOG_DEBUG(format,...) Log::get_instance->write_log(0,format,##__VA_ARGS__);Log::get_instance->flush();
#define LOG_INFO(format,...) Log::get_instance->write_log(1,format,##__VA_ARGS__);Log::get_instance->flush();
#define LOG_WARN(format,...) Log::get_instance->write_log(2,format,##__VA_ARGS__);Log::get_instance->flush();
#define LOG_ERROR(format,...) Log::get_instance->write_log(3,format,##__VA_ARGS__);Log::get_instance->flush();

#endif