/*
 * @Author       : WKq
 * @Date         : 2022/09/08
 * @Desc         : http连接类
 */ 

#ifndef _HTTPCONN_H
#define _HTTPCONN_H

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/uio.h>

#include <string>
#include <unordered_map>

#include "buffer.h"
#include "log.h"

class HttpConn {
public:
    typedef enum Parse_Status_ {    //HTTP解析请求状态
        Incomplete_req = 0,     //请求不完整
        Get_req,                //请求完成
        Bad_req,                //错误请求
        File_req,               //文件请求成功
        File_non,              //文件不存在
        Forbidden_req,          //访问权限不够
        Inner_error             //内部错误
    }Parse_Status;

    typedef enum State_Machine_ {
        Request_Line = 1,
        Request_head,
        Request_Content
    }State_Machine;

    static const std::unordered_map<int, std::string> CODE_STATUS;  //状态描述
    static const std::unordered_map<int, std::string> ERROR_MES;    //错误信息

public:
    HttpConn();
    ~HttpConn();
    void init(int fd_, struct sockaddr_in& addr_);        //初始化连接对象
    void init();
    void Close();                                         //关闭连接
    bool Onread();                                        //循环读取数据
    ssize_t Onwrite(int* Errno);                                       //循环写出数据
    Parse_Status ProcessRequest();                        //解析请求
    bool ProcessResponse(Parse_Status request_code);      //生成响应
    int Getfd();                                
    bool keepalive();

private:
    int get_content_length();
    Parse_Status parse_request_line(char* text);
    Parse_Status parse_request_head(char* text);
    Parse_Status parse_request_content(char* text);
    Parse_Status repare_resources();    //准备静态资源
    void add_response_line(int code);   
    void add_response_head(bool keepalive, size_t filesize);
    void add_response_content(int code);
    void tolower(std::string& str);
    void unmap();

public:
    static int conn_num;
    static const char* src; //资源路径

private:
    int fd; //连接套接字
    struct sockaddr_in caddr;   //客户端地址
    Buffer readbuf;             //读缓冲区
    Buffer writebuf;            //写缓冲区
    struct iovec iov[2];       //写空间
    int iv_count;

    State_Machine state_machine;    //状态机

    std::string method, path, version, body;      //请求行内容
    std::unordered_map<std::string, std::string> head;  //请求头
    int content_length; //请求体长度

    struct stat resource_stat;   //资源状态
    char* resource_addr;        //资源地址
};

#endif