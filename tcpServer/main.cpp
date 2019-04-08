#include <iostream>

extern "C"{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
 #include <arpa/inet.h>
}

#include <iostream>

using namespace std;

#define DEBUG 0

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
    if( (listenfd = socket(
                PF_INET,//ipv4
                SOCK_STREAM|//流 tcp
//                SOCK_NONBLOCK|//非阻塞
                SOCK_CLOEXEC//子进程继承带有该flag的fd时，子进程会默认关闭父进程的fd,
                ,IPPROTO_TCP//指定 协议，为0则为地址默认的协议
                ))
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

    struct sockaddr_in peerAddr;
    socklen_t peerLen = sizeof(peerAddr);
    while(true)
    {
        int connfd = accept4(listenfd, (struct sockaddr*)&peerAddr,
                             &peerLen,
//                             SOCK_NONBLOCK | //非阻塞
                             SOCK_CLOEXEC);

        if(connfd == -1)
            ERR_EXIT("accept4");

        std::cout <<  inet_ntoa( peerAddr.sin_addr) << "port:"<< ntohs(peerAddr.sin_port) << std::endl;
#if DEBUG
        while(1)
        {
            send(connfd, "233\n", sizeof("233\n"), 0);
        }
#endif

        while(true) //与连接进来的client通信，直到client发出退出请求
        {
            char buf[1024] = "";
            int readSize = sizeof(buf);
            int ret;

            std::cout << "- wait client msg -" <<endl;
            while( (ret = recv(connfd, buf, readSize, 0)) )//循环读，直到读不出来，返回0 (阻塞)
            {
                std::cout << buf << endl;
                if(ret <= readSize) //读完内容就break,否则又在等read的内容而卡住
                    break;
            }
            std::cout << "- got clietn msg -" <<endl;

            if(buf[0] == 'e' && buf[1] == 'i')//接收到退出指令，退出
            {
                send(connfd, "exited", sizeof("exited"), 0);
                close(connfd);
                break;
            }

            if(ret == -1)
                ERR_EXIT("read");

            string s = "<h1> hello world, server got msg</h1>";
            ret = send(connfd, s.c_str(), strlen(s.c_str()), 0) ;
            if(ret == -1)
                ERR_EXIT("send");
        }

//        close(connfd);
    }

}

void tcpClient(char* ip, char* port)
{
    sockaddr_in servAddr;
    std::cout << ip << ":" << port << std::endl;

    servAddr.sin_family = PF_INET;
    inet_aton(ip, &servAddr.sin_addr);
    servAddr.sin_port = htons(strtoul(port, 0, 0));

    int servfd;
    if( (servfd = socket(AF_INET,
                         SOCK_STREAM
//                         SOCK_NONBLOCK //非阻塞
                          , 0)) < 0 )
        ERR_EXIT("socket");

    while(true)
    {
        if( connect(servfd, (struct sockaddr*)&servAddr, sizeof(servAddr))  < 0 )
            ERR_EXIT("connect");

        while(true)
        {
            //默认client发信息， server收信息
            char buf[1024] = "";
            int bufSize = sizeof(buf);
            int ret;
            std::cout << "- input something to server:" << endl;
            std::cin.getline(buf, 1024, '\n');

            while( (ret = send(servfd, buf, bufSize, 0)) )
            {
                if(ret <= bufSize)
                    break;
            }

            write(servfd,  buf, sizeof(buf));
//             if(ret < 0)
//                 ERR_EXIT("send");

            while( (ret = recv(servfd, buf, sizeof(buf), 0)) )//阻塞时，循环读容易卡住，需要优化->可能需要非阻塞
            {
                std::cout << buf << endl;
                if(ret <= sizeof(buf))
                    break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if( argc == 3 )
        tcpClient(argv[1], argv[2]);
    else
        tcpServer( atoi(argv[1]));
    return 0;
}
