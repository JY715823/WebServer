#include <cstdio>
#include <pthread.h>
#include <queue> // 用于队列
#include <string>
#include <unistd.h>

// 创建队列
struct Q{
    std::queue<std::string> q; // 存储日志内容的队列
    bool stop = false; // 异步线程结束标志
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
}q;

// ****** 日志文件指针:[ 作为静态变量让不同线程进行共享 ]******
static FILE* fp = nullptr;

void* writer(void* args){
    // 一直循环把日志写入磁盘，直到停止标志为true
    while(true){
        // 先上锁
        pthread_mutex_lock(&q.mtx);
        // 等待条件变量
        /************************************************************
         * 队列为空并且线程不停止的时候阻塞
         * 线程一旦停止，就退出休眠，开始工作，直到把队列中剩余的内容消费完再退出 
         ************************************************************/
        while(q.q.empty() && !q.stop){
            pthread_cond_wait(&q.cv,&q.mtx);
        }
        // 当队列为空，并且停止条件为真时，安全退出
        if(q.q.empty() && q.stop) {
            // 先解锁再退出
            pthread_mutex_unlock(&q.mtx);
            break;
        }
        // 从队列中取出日志内容
        std::string s = q.q.front();
        q.q.pop();
        // ****** 立刻解锁:[ 操作临界区时只取出内容，IO操作在后面做，不要影响主线程入队 ]******
        pthread_mutex_unlock(&q.mtx);

        // 将日志内容写入磁盘，IO操作
        fputs(s.c_str(),fp);
        // 写入一个换行符
        fputc('\n',fp);
        // 刷新缓冲区
        fflush(fp);
    }

    return nullptr;
}

// ****** 返回值bool:[ 返回是否打成功，当日志线程停止后（stop = true）不应该再继续推送内容 ]******
bool async_log(std::string buf){
    // 获取锁
    pthread_mutex_lock(&q.mtx);
    // 将日志内容插入队列
    if(q.stop){
        // 先解锁再返回
        pthread_mutex_unlock(&q.mtx);
        return false;
    }
    q.q.push(buf);
    // 立刻解锁
    pthread_mutex_unlock(&q.mtx);
    // 通知消费者（写线程）
    pthread_cond_signal(&q.cv);

    return true;
}



int main(){
    // 打开文件指针
    fp = fopen("demo.log","a");
    if(!fp){
        perror("fopen error");
        return -1;
    }

    // 创建异步写日志的线程
    pthread_t t;
    pthread_create(&t,nullptr,&writer,nullptr);

    // 模拟打日志
    for(int i = 0;i < 920;i++){
        char buf[128] = {0};
        snprintf(buf,sizeof(buf),"log line %d",i);
        // 打日志
        if(!async_log(std::string(buf))){
            break;
        }
        usleep(10 * 1000);
    }

    // 关闭写线程
    pthread_mutex_lock(&q.mtx);
    q.stop = true;
    pthread_mutex_unlock(&q.mtx);
    pthread_cond_broadcast(&q.cv);

    // 等待线程退出
    pthread_join(t,nullptr);
    // 关闭文件指针
    fclose(fp);

    return 0;
}


