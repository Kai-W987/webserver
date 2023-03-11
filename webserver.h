/*
 * @Author       : WKq
 * @Date         : 2022/09/07
 * @Desc         : 服务器类
 */ 

#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include <unordered_map>
#include <memory>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "httpconn.h"
#include "threadpool.h"
#include "epoller.h"
#include "log.h"

class Webserver {
public:
    Webserver(uint16_t port_, size_t threadnum_ = 8);
    ~Webserver();
    void StartServer(); //启动服务器

private:
    bool InitSocket();                  //创建监听套接字
    void DealListen();                  //处理监听套接字
    void Dealread(HttpConn* client);    //处理连接的读事件，成功则唤醒工作线程
    void Dealwrite(HttpConn* client);   //处理写事件
    void Process(HttpConn* client);     //业务逻辑

    static int MAXFD;
    static void SetNonBlocking(int fd);

private:
    uint16_t port;                                   //端口
    int listenfd;                                    //监听套接字
    std::unordered_map<int, HttpConn> users;         //http连接对象
    char* src;                                       //默认资源路径
    bool isClose;                                    //是否关闭连接

    std::unique_ptr<ThreadPool> threadpool;          //线程池对象
    std::unique_ptr<Epoller> epoller;                //EPOLL对象
};

#endif