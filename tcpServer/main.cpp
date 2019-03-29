#include <iostream>

extern "C"{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
 #include <arpa/inet.h>
}

using namespace std;

#define ERR_EXIT(m)\
    do\
{\
    perror(m);\
    exit(EXIT_FAILURE);\
    }while(0)


int tcpServer(int port)
{
    //忽略 这两个信号
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

//    AF_INET == PF_INET

    int listenfd;
    if( listenfd = socket(
                PF_INET,//ipv4
                SOCK_STREAM|//流 tcp
//                SOCK_NONBLOCK|//非阻塞
                SOCK_CLOEXEC//子进程继承带有该flag的fd时，子进程会默认关闭父进程的fd,
                ,IPPROTO_TCP//指定 协议，为0则为地址默认的协议
                )
                < 0)
        ERR_EXIT("socket");
    struct sockaddr_in servAddr; //配置服务器地址
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;//协议族 ipv4
//    inet_aton() 将10进制.分ip地址转换为 网络制
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //设置
    servAddr.sin_port = htons(port);

    int on = 1;
    if( setsockopt(listenfd,
                   SOL_SOCKET, //在socket级别设置option
                   SO_REUSEADDR,//端口复用
                   &on,// 缓冲区指针
                   sizeof(on))
            < 0 )
        ERR_EXIT("setsockopt");

    //绑定 ip,端口
    if( bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr) ) <0 )
        ERR_EXIT("bind");

    //监听
    if( listen(listenfd, SOMAXCONN) < 0 )//监听个数
        ERR_EXIT("listen");

    accept4(listenfd, )

    while(true)
    {

    }

}

int main(int argc, char *argv[])
{


    return 0;
}
