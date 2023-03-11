/*
 * @Author       : WKq
 * @Date         : 2022/09/07
 * @Desc         : 自增长缓冲区类
 */ 

#ifndef _BUFFER_H
#define _BUFFER_H

#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>

#include <string>

class Buffer {
public:
    typedef enum Line_Status_ {
        Get_Line = 1,
        Bad_Line,
        Incomplete_Line
    }Line_Status;

public:
    Buffer(size_t size_ = 1024);
    ~Buffer();

    ssize_t Read(int fd, int* Errno);
    void Append(std::string str);
    Line_Status Parse_Line();           //根据\r\n解析一行数据
    char* get_line();
    void set_parselength(size_t len = 0);
    size_t Readable_length();
    void Add_readpos();                 //readpos前移
    void Clear();                       //清空缓冲区，但空间不变

private:
    void Append(const char* buf, size_t len);

private:
    char* m_buffer;         //缓冲区
    size_t size;           //缓冲区长度
    size_t writepos;       //写入位置
    size_t readpos;        //读取位置
    size_t parselength;       //解析长度
};

#endif