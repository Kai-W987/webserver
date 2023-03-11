/*
 * @Author       : WKq
 * @Date         : 2022/08/25
 * @Desc         : 日志类
 */ 

#ifndef _LOG_H
#define _LOG_H

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include <stdarg.h>

#include "locker.h"

#define DEBUG 1

class Log {
public:
    bool init(const char* filename = "./log/1.log", const long MaxLongSize = 10);
    void Write(const char* fmt, ...);
    void Addtime();
    void Close();
    ~Log();

private:
    FILE* Fopen(const char* filename, const char* mode);
    bool Mkdir(const char* filename);
    void Localtime(char* strtime);

private:
    Log();

public:
    static MyMutex m_mutex;
    static Log* instance;
    static Log* getinstanc();

private:
    FILE* m_fp;
    const char* m_filename;     //文件路径
    long m_MaxLogSize;
    MyMutex w_mutex;
};

#if DEBUG

#define LOG(type, fmt, ...) do{\
    Log* log = Log::getinstanc();\
    log->Addtime();\
    log->Write("[%s]<%s, %s:%d>:", type, __FILE__, __FUNCTION__, __LINE__);\
    log->Write(fmt, ##__VA_ARGS__);\
}while(0)

#define Info(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define Debug(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#define Error(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)

#else

#define LOG(type, fmt, ...)
#define Debug(fmt, ...) 
#define Error(fmt, ...)

#endif

#endif