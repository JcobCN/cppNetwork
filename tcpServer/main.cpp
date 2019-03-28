#include <iostream>

extern "C"{
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
}

using namespace std;

#define ERR_EXIT(m)\
    do\
{\
    perror(m);\
    exit(EXIT_FAILURE);\
    }while(0)


int tcpServer()
{
    //忽略 这两个信号
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

//    AF_INET == PF_INET

    int listenfd;
    if( socket(
                PF_INET,//ipv4
                SOCK_STREAM|//流 tcp
                SOCK_NONBLOCK|//非阻塞
                SOCK_CLOEXEC//子进程继承带有该flag的fd时，子进程会默认关闭父进程的fd,
                ,IPPROTO_TCP//指定 协议，为0则为地址默认的协议
                )
                < 0)
        ERR_EXIT("socket");
    struct sockaddr_in servAddr;
}

int main(int argc, char *argv[])
{


    return 0;
}
