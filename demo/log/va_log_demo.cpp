#include <cstdarg> // 可变参数要用到（va_list/va_start/va_end）
#include <cstdio>
#include <cstring>

// 将printf风格的（fmt,...）格式化到buf之中
static void format_log(char* buf,size_t cap,char* format,...){
    // 指向可变参数
    va_list ap;
    va_start(ap,format); // ap指向可变参数起点

    // ****** vsnprintf:[ 把ap指向的可变参数列表按format格式写入buf中 ]******
    // 返回写入的字符数（不含'\0'），如果截断也会返回实际应写入的长度
    int n = vsnprintf(buf,cap,format,ap);
    if(n < 0){
        printf("格式化失败!\n");
    }else if(n >= cap){
        printf("发生截断，但末尾会保留\\0\n");
    }else{
        printf("格式化成功!\n");
    }

    // 必须收尾
    va_end(ap);

}


int main(){
    char buf[128];
    format_log(buf,sizeof(buf),"user=%s id=%d pi=%.2f","jy",7,3.14159);
    printf("buf=%s\n",buf);

    return 0;
}