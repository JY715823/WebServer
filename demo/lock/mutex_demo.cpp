#include <iostream>
#include <pthread.h>

using namespace std;

static long long g = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void* worker(void*){
    for(int i = 0;i < 10000000;i++){
        pthread_mutex_lock(&mtx);
        g++;
        pthread_mutex_unlock(&mtx);
    }
    // ******[ 这里必须要添加返回值nullptr,因为返回值是void*，要显式返回 ]******
    return nullptr;
}


int main(){
    pthread_t t1,t2;

    pthread_create(&t1,nullptr,worker,nullptr);
    pthread_create(&t2,nullptr,worker,nullptr);

    pthread_join(t1,nullptr);
    pthread_join(t2,nullptr);
    cout << g << endl;

    return 0;
}

/************************************************************
    int pthread_create(
    pthread_t* thread,
    const pthread_attr_t* attr,
    void* (*start_routine)(void*),
    void* arg
);
第 2 个参数 nullptr：线程属性 attr

attr 用来指定线程属性（栈大小、是否 detached、调度策略等）。
传 nullptr 表示 使用默认线程属性（最常见）。
如果你想设置成“分离线程”（不需要 join），会这样写：
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_create(&t1, &attr, worker, nullptr);
pthread_attr_destroy(&attr);

 ************************************************************/

/************************************************************
  pthread_join(t1, nullptr);
  第 2 个参数 nullptr：返回值接收位置 retval
pthread_join 可以拿到线程函数 worker 的返回值（也就是 worker 返回的 void*）。
传 nullptr 表示：我不关心线程返回了什么，只要等待它结束即可。
 ************************************************************/

