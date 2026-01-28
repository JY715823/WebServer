#include <cstdio>
#include <sys/time.h> // 用于gettimeofday
#include <ctime> // 用于localtime_r

int main(){
    // 得到时间戳
    timeval tv = {};
    gettimeofday(&tv,NULL);

    // 得到日历时间（用tv的秒来计算年月日、时分秒等）
    tm tm_now = {};
    localtime_r(&tv.tv_sec,&tm_now);

    char prefix[128];
    int n = snprintf(prefix,sizeof(prefix),
        "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
        tm_now.tm_year + 1900,tm_now.tm_mon + 1,tm_now.tm_mday,
        tm_now.tm_hour,tm_now.tm_min,tm_now.tm_sec,
        static_cast<long>(tv.tv_usec));

    printf("prefix : %s\n",prefix);

    return 0;
}