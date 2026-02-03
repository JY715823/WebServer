#include "log.h" 

Log::Log(){
    m_count = 0;
    m_is_async = false;
}
Log::~Log(){
    // 如果是异步模式，先等待线程结束
    if(m_is_async && m_q){
        m_q->close();
        pthread_join(m_tid,nullptr);
        // 释放队列空间
        delete m_q;
        m_q = nullptr;
    }

    m_mtx.lock();
    // 关闭文件指针
    if(m_fp){
        fclose(m_fp);
        m_fp = nullptr;
    }
    m_mtx.unlock();
}

// 异步写日志到磁盘
void Log::async_write_log(){
    // 循环跑，直到日志系统关闭
    string s_log;
    // 从队列中取出日志内容
    while(m_q->pop(s_log)){
        // 写日志到磁盘
        m_mtx.lock();
        fputs(s_log.c_str(),m_fp);
        m_mtx.unlock();
    }
}

// ****** 懒汉单例模式:[ 第一次使用到Log的时候再对唯一的实例进行初始化 ]******
Log* Log::get_instance(){
    // ****** 局部静态变量实现单例模式:[ C++11之后不用再加锁,详情见笔记 ]******
    
    // ****** 用new的漏洞:[ 几乎不会有机会delete进行析构,会造成泄露 ]******
    // static Log* instance = new Log();
    // return instance;

    // ****** 更推荐的做法:[ 在栈区创建 ]******
    static Log instance;
    return &instance;
}

// ****** 启动异步日志工作线程:[ 作为桥接函数,具体见static函数的作用 ]******
void* Log::start_log_thread(void* args){
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // [FIXED BUG]: 静态成员函数只能访问静态成员变量或者传实例化对象
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Log::get_instance()->async_write_log();
    return nullptr;
}

// 获取当前日历时间
void Log::get_time(struct timeval& tv,struct tm& now){
    // 得到当前时间戳
    gettimeofday(&tv,nullptr);
    // 用时间戳计算当前日历时间
    localtime_r(&tv.tv_sec,&now); // 保证线程安全
}


// ****** 对日志系统进行初始化:[ 可通过外部传参来控制相关参数,没有传参就使用默认参数 ]******
bool Log::init(const char* file_name,int close_log,int log_buf_size,
            int split_lines,int max_queue_size){
    // 异步模式开关
    if(max_queue_size >= 1){
        // 开启异步模式
        m_is_async = true;
        // 为阻塞队列申请空间
        m_q = new block_queue<string>(max_queue_size);
        // 创建异步日志的工作线程
        int res = pthread_create(&m_tid,nullptr,&start_log_thread,nullptr);
        if(res != 0){
            // 回滚状态
            m_is_async = false;
            delete m_q; m_q = nullptr;
            return false;
        } 
    }

    // 保存配置,初始化缓冲区
    m_log_buf_size = log_buf_size;
    m_close_log = close_log;
    m_split_lines = split_lines;

    // 取当前日期
    struct timeval tv;
    struct tm now;
    get_time(tv,now);

    // 根据filename生成完整文件名
    /************************************************************
     * 从后往前找到/,看文件名是否带目录路径
     * 如果找到了,就将目录名和文件名一起,作为要打开的日志文件名时间前缀之后的内容
     * 如果没找到,就只把文件名作为日志文件名
     ************************************************************/
    const char* p = strrchr(file_name,'/');
    const char* logName = nullptr;
    // 完整的文件名(带时间前缀)
    char fullFileName[256] = {0};
    if(p != nullptr){
        // 参数带有目录名
        // 获取目录路径
        int len = p - file_name + 1;// 获取长度
        strncpy(dir_name,file_name,len);
        // ****** 手动加\0:[ 如果过长是不会自动加\0的 ]******
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // [FIXED BUG]: 要用len,不要用sizeof(dir_name) - 1,不然一堆乱码
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        dir_name[len] = '\0';
        logName = p + 1;
        strncpy(log_name, p + 1, sizeof(log_name) - 1);
        log_name[sizeof(log_name) - 1] = '\0';
        // 加上时间前缀
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // [FIXED BUG]: 这些时间类型是整型,不能用%s,要用%d
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        snprintf(fullFileName,sizeof(fullFileName),"%s%04d_%02d_%02d_%s",dir_name,
            now.tm_year + 1900,now.tm_mon + 1,now.tm_mday,logName);
    }else{
        dir_name[0] = '\0';
        logName = file_name;
        strncpy(log_name, file_name, sizeof(log_name) - 1);
        log_name[sizeof(log_name) - 1] = '\0';
        snprintf(fullFileName, sizeof(fullFileName), "%04d_%02d_%02d_%s", 
                 now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, logName);
    }

    // 记录今天几号(按天切分日志)
    m_today = now.tm_mday;

    // 用文件指针打开日志文件
    m_fp = fopen(fullFileName,"a");
    if(!m_fp){
        return false;
    }
    return true;
}


// ****** 日志系统核心:[ 将日志通过同步/异步的方式写入磁盘持久化 ]******
void Log::write_log(int level,const char* format,...){
    // 1.检查是否需要分文件（隔天、超出最大行数限制）
    struct timeval tv = {};
    struct tm now = {};
    get_time(tv,now);
    // 上锁，检查当天时间是否和m_today相同
    m_mtx.lock();
    char newFileName[256] = {0};
    if(now.tm_mday != m_today){
        // 不是同一天，需要分文件
        // 将新的时间拿来组成新的文件名
        snprintf(newFileName,sizeof(newFileName),"%s%04d_%02d_%02d_%s",dir_name,now.tm_year + 1900,
            now.tm_mon + 1,now.tm_mday,log_name);
        // 将行数计数归零
        m_count = 0;
        // 更新今天
        m_today = now.tm_mday;
    }else if(m_count && m_count % m_split_lines == 0){
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // [FIXED BUG]: 新增判断:m_count，不然一开始启动的时候会误认为需要切分
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // 将分割的第几个文件数数组成新的文件名
        snprintf(newFileName,sizeof(newFileName),"%s%04d_%02d_%02d_%s.%d",dir_name,now.tm_year + 1900,
            now.tm_mon + 1,now.tm_mday,log_name,m_count / m_split_lines);
    }
    // 打开新的日志文件
    if(strlen(newFileName) != 0){
        // 打开新文件
        FILE* newFp = fopen(newFileName,"a");
        if(newFp){
           // 关闭旧的文件
           // ****** fclose:[ 不用手动调用fflush,fclose会强制刷新缓冲区剩余内容再关闭文件描述符 ]******
           fclose(m_fp);
           m_fp = newFp; 
           newFp = nullptr;
        }
    }
    // 更新日志行数
    m_count++;
    // 及时解锁
    m_mtx.unlock();


    // 定义写入日志的字符串,以及临时字符数组保存各内容
    string s_log;
    s_log.reserve(m_log_buf_size);
    char temp_buf[m_log_buf_size] = {0};
    // 将日期时间写入
    snprintf(temp_buf,sizeof(temp_buf),"%04d-%02d-%02d %02d:%02d:%02d.%06ld ",now.tm_year + 1900,now.tm_mon + 1,
            now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec,tv.tv_usec);
    s_log += temp_buf;

    // 2.根据level将日志级别的内容拼接上去
    memset(temp_buf,0,sizeof(temp_buf));
    switch(level){
        case 0: strcpy(temp_buf, "[debug]:"); break;
        case 1: strcpy(temp_buf, "[info]:");  break;
        case 2: strcpy(temp_buf, "[warn]:");  break;
        case 3: strcpy(temp_buf, "[error]:");  break;
        default:strcpy(temp_buf, "[info]:");  break;
    }
    s_log += temp_buf;


    // 3.把日志的主要内容拼接上去
    // 获取日志主要内容
    memset(temp_buf,0,sizeof(temp_buf));
    // 定义可变参数列表
    va_list vl;
    // 启动可变参数列表，让其指向开头
    va_start(vl,format);
    // 拼接格式和可变参数
    vsnprintf(temp_buf,sizeof(temp_buf),format,vl);
    // 关闭可变参数列表
    va_end(vl);
    s_log += temp_buf;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // [FIXED BUG]: 写入内容加入换行
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    s_log.push_back('\n');

    // 4.同步/异步写日志
    // 判断同步/异步写日志
    if(m_is_async && !m_q->isFull()){
        // 将日志内容加入到阻塞队列中,不进行写入
        m_q->push(s_log);
    }else{
        // 同步写日志，需要上锁，因为使用相同的文件指针，否则会被覆盖
        m_mtx.lock();
        fputs(s_log.c_str(),m_fp);
        m_mtx.unlock();
    }
}


// 强制刷新日志文件指针缓冲区到内核里
void Log::flush(){
    /************************************************************
     * fflush 不是“强制落盘”
        它通常只保证写到内核页缓存，并不保证写到磁盘。
        真要更强保证：fsync(fileno(fp))（性能代价很大，日志一般不这么干） 
     ************************************************************/
    m_mtx.lock();
    fflush(m_fp);
    m_mtx.unlock();
}


