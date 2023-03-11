#include "webserver.h"

/*
    int listenfd;                               //监听套接字
    std::unordered_map<int, HttpConn> users;    //http连接对象
    char* src;                                  //默认资源路径
    bool isClose;                               //是否关闭连接

    std::unique_ptr<ThreadPool> threadpool;     //线程池对象
*/

int Webserver::MAXFD = 65535;

Webserver::Webserver(uint16_t port_, size_t threadnum_) :port(port_), isClose(false),\
                    threadpool(new ThreadPool(threadnum_)), epoller(new Epoller) {
    src = getcwd(NULL, 256);    // 当前目录
    //cout << src << endl;
    assert(src);
    strncat(src, "/resources", 12);
    //printf ("%s\n", src);

    HttpConn::conn_num = 0;
    HttpConn::src = src;

    if (!InitSocket()) {
        isClose = true;
    }

    if (Log::getinstanc()->init("./log/webserver.log")) {
        if (isClose) {
            Debug("服务器启动失败...\n");
        }else {
            Debug("服务器启动成功...\n");
            Debug("工作目录: %s, 端口号：%d\n", src, port);
        }
    }
} 

Webserver::~Webserver() {
    close(listenfd);
    isClose = true;
    Debug("服务器关闭...");
}

void Webserver::StartServer() {
    while (!isClose) {
        int ready_num = epoller->Wait(-1);

        //轮询就绪事件
        for (int ii = 0; ii < ready_num; ii++) {
            int fd = epoller->Getfd(ii);
            uint32_t events = epoller->Getevent(ii);

            if (fd == listenfd) {   //监听套接字
                DealListen();
            }else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {    //关闭连接
                epoller->Delfd(fd);
                users[fd].Close();
            }else if (events & EPOLLIN) {
                Dealread(&users[fd]);
            }else if (events & EPOLLOUT) {
                Dealwrite(&users[fd]);
            }

        }
    }
}


bool Webserver::InitSocket() {
    if (port > 65535 || port < 1024) {  //0~1023为知名端口
        return false;   //不在可用端口范围，则初始化失败
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);     //TCP字节流协议
    
    if (listenfd < 0) {
        return false;
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);

    //端口复用
    socklen_t optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    //绑定
    int ret = bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (ret < 0) {
        return false;
    }

    //监听
    listen(listenfd, 6);

    //加入epoll
    if ( !(epoller->Addfd(listenfd, EPOLLIN, false)) ) {
        close(listenfd);
        return false;
    }

    //设置非阻塞
    SetNonBlocking(listenfd);

    return true;
}

void Webserver::DealListen() {
    //printf ("DealListen\n");
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);

    while (true) {
        int fd = accept(listenfd, (struct sockaddr *)&caddr, &len);
        
        if (fd <= 0) {
            return;
        }

        if (HttpConn::conn_num >= MAXFD) {
            close(fd);
            Error("Can't connect more!!!");
            continue;
        }

        users[fd].init(fd, caddr);    //初始化连接对象
        epoller->Addfd(fd, EPOLLIN, true);  //注册读事件

        SetNonBlocking(fd); //设置非阻塞
        
#if DEBUG
        char client_ip[16] = {0};
        unsigned short client_port = ntohs(caddr.sin_port);
        inet_ntop(AF_INET, &caddr.sin_addr.s_addr, client_ip, 16);
#endif

        Debug("新客户端连接... ip = %s, port = %d, fd = %d\n", client_ip, client_port, fd);
    }
}

void Webserver::Dealread(HttpConn* client) {
    assert(client);
    //printf ("Dealread\n");

    if (client->Onread()) { //TCP窗口数据全部读取，由工作线程解析请求
        threadpool->AddTask(std::bind(&Webserver::Process, this, client));  //绑定成无参函数对象
    }else { //读取失败关闭连接
        epoller->Delfd(client->Getfd());
        client->Close();    
    }
}

void Webserver::Dealwrite(HttpConn* client) {   //非阻塞写
    int Errno;
    ssize_t ret = client->Onwrite(&Errno);

    if (ret <= -1) {
        if (Errno == EAGAIN) {
            epoller->Modfd(client->Getfd(), EPOLLOUT);
        }else {
            epoller->Delfd(client->Getfd());
            client->Close();
        }
        return;
    }else if (ret == 0 && client->keepalive()) {
        client->init();
        epoller->Modfd(client->Getfd(), EPOLLIN);
        return;
    }

    epoller->Delfd(client->Getfd());
    client->Close();
}

void Webserver::Process(HttpConn* client) {
    //printf ("Process\n");
    HttpConn::Parse_Status request_code = client->ProcessRequest();

    if (request_code == HttpConn::Incomplete_req) {    //http请求不完整
        epoller->Modfd(client->Getfd(), EPOLLIN);
        return;
    }

    bool response_ret = client->ProcessResponse(request_code); //生成响应
    if (!response_ret) {
        epoller->Delfd(client->Getfd());
        client->Close();
        Error("响应生成错误\n");
        return;
    }

    epoller->Modfd(client->Getfd(), EPOLLOUT);
}

void Webserver::SetNonBlocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

