#include <iostream>
#include <string>

extern "C"{
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
}

using std::string;
using std::cout;
using std::endl;

using namespace std;
enum SelectState{
    ALL = 0,
    SelectRead = 1,
    SelectWrite,
    SelectExecpt,
    SelectError = -1
};

class HTTP{
public:
    void setIpPort(string ip, string port);
    void setHeader(int limit, int offset);
    int sendMsg();
    int recvMsg(char *data, int dataSize);
    int recvHttpMsg(char* recvBuf, int recvBufSize);
    SelectState socketWaitForRW(int sockFd, SelectState concern, int timeoutSec);
    virtual ~HTTP();
private:
    string header;
    string ip;
    string port;
    int servfd;
};


int HTTP::sendMsg()
{
    sockaddr_in servAddr;
    std::cout << ip << ":" << port << std::endl;

    servAddr.sin_family = PF_INET;
    inet_aton(ip.c_str(), &servAddr.sin_addr);
    servAddr.sin_port = htons( strtol(port.c_str(), 0, 0) );

    servfd = -1;
    servfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    if(servfd < 0)
    {
        perror("socket");
        return 0;
    }

    int ret = -1;
    do{
        ret = connect(servfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
        if(ret == 0)//connect completed immediately!! without select.
        {
            break;//已连接上服务器，跳过select，直接send
        }else{//出错-1
            if(errno != EINPROGRESS){
                perror("connect");
                // close(servfd);
                return -1;
            }
            //EINPROGRESS 正在连接中，通过select判断是否可写。
            printf("EINPROGRESS\n");
        }

        ret = socketWaitForRW(servfd, SelectWrite, 2);
        if(ret < 0)
            return -1;
        else if( ret != SelectWrite )
            return -1;
    }while(0);


    char buf[2048] = "";
    bzero(buf, sizeof(buf));
    strcpy(buf, header.c_str());

    int dataSize = strlen(buf);
    int totalSend = 0;

    //ret 已发送的字节数
    ret = 0;
    do{
        ret = send(servfd, buf+ret, dataSize-ret, 0);
        if(ret < 0 )//出错
        {
            perror("send");
            // close(servfd);
            return -1;
        }

        totalSend += ret;
        if( totalSend >= dataSize )
            break;
    }while(1);

    cout << "start get" << endl;
    cout << buf << endl;
    cout << "get~" << endl;
    cout << endl;

    bzero(buf, sizeof(buf));
}

int HTTP::recvMsg(char* data, int dataSize)
{

    int ret = -1;
    ret = socketWaitForRW(servfd, SelectRead, 2);
    if(ret != SelectRead)
    {
        printf("socketWaitForRW ret = %d\n", ret);
        return -1;
    }

    bzero(data, sizeof(dataSize));
    ret = recvHttpMsg(data, dataSize);
    if(ret < 0)
    {
        printf("recvHttpMsg ret = %d\n", ret);
        return -1;
    }

    cout << "got header msg" << endl;
    printf("recvBuf=%s\n", data);

    // close(servfd);//临时调试，get http头
    // return 0;

    ret = socketWaitForRW(servfd, SelectRead, 2);
    if(ret != SelectRead)
    {
        printf("socketWaitForRW ret = %d\n", ret);
        return -1;
    }
    bzero(data, sizeof(dataSize));
    ret = recvHttpMsg(data, dataSize);
    if(ret < 0)
        return -1;

    cout << "got chunked msg" << endl;
    printf("recvBuf=%s\n", data);

    // close(servfd);
}

int HTTP::recvHttpMsg(char *recvBuf, int recvBufSize)
{
    int i = 0;
    int ret = -1;
    do
    {
        ret = recv(servfd, &recvBuf[i], 1, 0);
       
        if(ret == -1)//出错
        {
            printf("errno = %d\n", errno);
            switch(errno)
            {
            case EINTR: //等系统中断，继续循环
                continue;
                break;
            case EAGAIN:
                continue;
            default:
                return -1;
            }
        }

        if( recvBuf[i] == '\n' )//查找\r\n\r\n 尾部分隔符
        {
            if( i >= 3 ){
                if( recvBuf[i-1] == '\r' && recvBuf[i-2] == '\n' && recvBuf[i-3] == '\r')
                {
                    i>=recvBufSize ? recvBuf[recvBufSize-1] = '\0' : recvBuf[i+1] = '\0';
                    break;
                }
            }
        }

        i++;
        if(i>=recvBufSize)
        {
            recvBuf[recvBufSize-1] = '\0';
            break;
        }
    }while(1);

    return 0;
}

//须设置关心的集合，例如写集合或读集合
SelectState HTTP::socketWaitForRW(int sockFd, SelectState concern, int timeoutSec)
{
    fd_set readFds;//读操作符
    FD_ZERO(&readFds);
    FD_SET(sockFd, &readFds);

    fd_set excepFds;//写操作符
    FD_ZERO(&excepFds);
    FD_SET(sockFd, &excepFds);

    fd_set writeFds;//异常操作符
    FD_ZERO(&writeFds);
    FD_SET(sockFd, &writeFds);

    fd_set *rFd = NULL;
    fd_set *wFd = NULL;
    fd_set *exFd = NULL;

    switch(concern){
    case ALL:
        rFd = &readFds;
        wFd = &writeFds;
        exFd = &excepFds;
        break;
    case SelectRead:
        rFd = &readFds;
        break;
    case SelectWrite:
        wFd = &writeFds;
        break;
    case SelectExecpt:
        exFd = &excepFds;
        break;
     default:
        return SelectError;
    }


    struct timeval timeout = {timeoutSec, 0};

    //			设为FD_SET 设置的fd中最大的+1 提高select查找效率，否则会从0查找到FD_SIZEl
    int ret = select(sockFd+1, rFd, wFd, exFd,  &timeout);
    // >0 返回对应fd位为1的总数（事件发生，未发生的对应位被清空）
    // == 0 timeout
    // ==-1 error
    if(ret < 0)
    {
        perror("select");
        return SelectError; }
    else if(ret == 0)
    {
        perror("connect timeout");
        return SelectError;
    }

    printf("selcet ret=%d\n", ret);



    if( wFd && rFd && FD_ISSET(sockFd, wFd) && FD_ISSET(sockFd,rFd))
    {
        printf("connect error \n");
        return SelectError;
    }

    switch(concern)
    {
    case SelectWrite:
        if(FD_ISSET(sockFd,wFd))
        {
            printf("write available\n");
            return SelectWrite;
        }
        else{
            printf("write error\n");
            return SelectError;
        }
        break;
    case SelectRead:
        if(FD_ISSET(sockFd,rFd))
        {
            printf("read available\n");
            return SelectRead;
        }else
        {
            printf("read error\n");
            return SelectError;
        }
        break;
    case SelectExecpt:
        if(FD_ISSET(sockFd, exFd))
        {
            perror("select exception");
            return SelectExecpt;
        }else return SelectError;
        break;
    default:
        printf("else .. select option\n");
        return SelectError;
        break;
    }

    return SelectError;
}

HTTP::~HTTP()
{
    close(servfd);
}


void HTTP::setIpPort(string ip, string port)
{
    this->ip = ip;
    this->port = port;
}

void HTTP::setHeader(int limit, int offset)
{
    char buf[64];
    sprintf(buf, "%d", limit);
    string lm = buf;

    sprintf(buf,"%d", offset*limit);
    string of = buf;

    this->header = "GET http://"+ip+":"+port+"/cgi-bin/rserver.cgi?action=256&user=admin&qtype=6&limit="+lm+"&offset="+of+"&getall=0&filter=0 HTTP/1.1\r\n"
         "Host: "+ip+":"+port+"\r\n"
         "Connection: Close\r\n"
         "Accept: */*\r\n"
         "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.221 Safari/537.36 SE 2.X MetaSr 1.0\r\n"
         "Accept-Language: zh-CN,zh;q=0.8\r\n\r\n";
}

class RserverListUserInfo
{
public:
    static int getTotal(char* buf);
    int parseData(char* buf, int num); // start 1
private:
    int ubid;
    char account[128];
    char name[128];
    char passwd[128];
    char num[128];
    char contact[128];
    char telepone[128];
    char mailbox[128];
    char province[128];
    char city[128];
    char district[128];
    char location[128];
    char install[128];
    char curSelTime[128];
    char rTime[128];
    char userType[128];
    char userStatus[128];
};


int main(int argc, char *argv[])
{
    HTTP a;

    char dataBuf[5200];

    a.setIpPort("192.168.11.90", "5080");
    a.setHeader(12, 4);
    int ret = a.sendMsg();
    if(ret == -1)
        return -1;

    ret = a.recvMsg(dataBuf, sizeof(dataBuf));
    if(ret == -1)
        return -1;

    cout << RserverListUserInfo::getTotal(dataBuf) << endl;

    RserverListUserInfo b;
    b.parseData(dataBuf, 1);
}

int RserverListUserInfo::getTotal(char *buf)
{
    char *p = NULL;
    char *q = NULL;
    p = strstr(buf, "\"total\"");
    q = strchr(p, ',');

    char str[128] = "";
    strncpy(str, p, q-p);
    printf("str=%s\n", str);

    char num[64] = "";

    sscanf(str, "\"total\":\"%s\"", num);
    return atoi(num);

}

int RserverListUserInfo::parseData(char *buf, int num)
{
    char *p = strchr(buf, '[');
    char *q = NULL;

    int count = 0;
    while(1)
    {
        p = strchr(p, '{');
        count++;
        if(count == num)
            break;

        p++;
    }
    q = strchr(p, '}');
    q++;

    char str[512] = "";
    char str2[512] = "";
    strncpy(str, p, q-p);

    int i = 0;
    int j = 0;
    while(str[i] != '\0')
    {
        if(str[i] != '"')
            str2[j++] = str[i];

        i++;
    }

    printf("info=%s\n", str2);

    sscanf(str2, "{ubid:%d,"
           "userName:%[^ ,],"
           "usernickName:%[^ ,],"
           "userPasswd:%[^ ,],"
           "userNum:%[^ ,],"
           "contact:%[^ ,],"
           "telephone:%[^ ,],"
           "mailbox:%[^ ,],"
           "addrProvince:%[^ ,],"
           "addrCity:%[^ ,],"
           "addrDistrict:%[^ ,],"
           "addrLocation:%[^ ,],"
           "addrInstall:%[^ ,],"
           "curSelTime:%[^ ,],"
           "RTime:%[^a-zA-Z,],"
           "userType:%[^ ,],"
           "userStatus:%[^ ,}]}"
           ,
           &ubid,
           account,
           name,
           passwd,
           this->num,
           contact,
           telepone,
           mailbox,
           province,
           city,
           district,
           location,
           install,
           curSelTime,
           rTime,
           userType,
           userStatus);

    printf("ubid=%d\n account=%s\n name=%s\n passwd=%s\n num=%s\n contact=%s\n telepone=%s\n mailbox=%s\n province=%s\n city=%s\n district=%s\n location=%s\n install=%s\n curSelTime=%s\n rTime=%s\n userType=%s\n userStatus=%s\n",
           ubid,
           account,
           name,
           passwd,
           this->num,
           contact,
           telepone,
           mailbox,
           province,
           city,
           district,
           location,
           install,
           curSelTime,
           rTime,
           userType,
           userStatus);

    return 0;
}
