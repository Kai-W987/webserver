#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "webserver.h"

static void dameon(bool noclose);

int main() {
    dameon(false);

    Webserver web(5200, 8);

    web.StartServer();

    return 0;
}

/***************************************************************************
 * 创建守护进程
 * 输入：
 *      noclose - 是否关闭标准输入、输出、错误，true不关闭、false关闭
 * 返回值：
 *      无
 **************************************************************************/
void dameon(bool noclose) {
    switch(fork()) {
        case -1: {
            exit(-1);
        }
        case 0: {   //子线程作为后台程序
            break;
        }
        default: {  //主线程退出
            exit(0);
        }
    }

    if (setsid() == -1) {   //创建新会话，是程序脱离终端
        exit(-1);
    }

    int fd;
    if (!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);     //写入 /dev/null 的数据会被丢弃，重定向标准输入、输出、错误
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }
}