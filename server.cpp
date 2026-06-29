#include<bits/stdc++.h>
#include<iostream>
#include<unistd.h>
#include<cstring>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<fcntl.h>

using namespace std;

int MAX_CONNECTIONS=64;

void makeNonBlock(int fd)
{
    if(fd<0)
    return;

    int flag=fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
}
void printError()
{
    cout<<strerror(errno)<<endl;
}
enum class RespType
{
    ARRAY,
    BULK_STRING,
    SIMPLE_STRING,
    INTEGER,
    ERROR,
    EMPTY,
    INVALID

};
class Client
{
    public:
    int fd;
    string inputBuffer;
    Client()
    {
        
    }
    Client(int fd)
    {
        this->fd=fd;
    }

};
RespType getRespType(string &inputBuffer)
{
    if(inputBuffer.size()==0 || inputBuffer[0]=='\n')
    return RespType::EMPTY;

    if(inputBuffer[0]=='*')
    return RespType::ARRAY;

    if(inputBuffer[0]=='$')
    return RespType::BULK_STRING;

    if(inputBuffer[0]=='+')
    return RespType::SIMPLE_STRING;

    if(inputBuffer[0]==':')
    return RespType::INTEGER;

    if(inputBuffer[0]=='-')
    return RespType::ERROR;

    return RespType::INVALID;
}
int main()
{
    int serverFd=socket(AF_INET,SOCK_STREAM,0);
    unordered_map<int,Client> hash;

    if(serverFd<0)
    {
        printError();
        return 0;
    }
    cout<<"Socket Created\n";
    makeNonBlock(serverFd);

    sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(8080);
    serverAddr.sin_addr.s_addr=INADDR_ANY;

    if(bind(serverFd,(sockaddr*)&serverAddr,sizeof(serverAddr))<0)
    {
        printError();
        close(serverFd);
        return 0;
    }
    cout<<"SocketBind Success\n";
    listen(serverFd,MAX_CONNECTIONS);

    int epfd=epoll_create1(0);
    if(epfd<0)
    {
        printError();
        close(serverFd);
        return 0;
    }
    cout<<"Epoll Instance Created\n";

    epoll_event ev;
    ev.events=EPOLLIN;
    ev.data.fd=serverFd;
    if(epoll_ctl(epfd,EPOLL_CTL_ADD,serverFd,&ev)<0)
    {
        printError();
        close(epfd);
        close(serverFd);
        return 0;
    }
    cout<<"Epoll Watching Listening Socket\n";
    char buffer[1028]={0};
    epoll_event events[MAX_CONNECTIONS];

    while(1)
    {
        int n=epoll_wait(epfd,events,MAX_CONNECTIONS,-1);
        for(int i=0;i<n;i++)
        {
            int fd=events[i].data.fd;

            if(fd==serverFd)
            {
                cout<<"Connection Request Recived\n";
                int clientFd=accept(serverFd,nullptr,nullptr);
                if(clientFd<0)
                {
                    printError();
                    continue;
                }
                makeNonBlock(clientFd);
                epoll_event ev;
                ev.events=EPOLLIN;
                ev.data.fd=clientFd;
                if(epoll_ctl(epfd,EPOLL_CTL_ADD,clientFd,&ev)<0)
                {
                    printError();
                    close(clientFd);
                    continue;
                }
               
                hash[clientFd]=Client(clientFd);
                cout<<clientFd<<"  Created And Being Watched By Epoll\n";
            }
            else{
                Client &user=hash[fd];
                memset(buffer,0,sizeof(buffer));
                int bytes=recv(fd,buffer,sizeof(buffer),0);
                if(bytes>0)
                {
                    while(bytes>0)
                    {
                        string mess(buffer,bytes);
                        user.inputBuffer+=mess;
                        memset(buffer,0,sizeof(buffer));
                        bytes=recv(fd,buffer,sizeof(buffer),0);
                        

                    }
                    RespType val=getRespType(user.inputBuffer);
                    switch (val)
                    {
                        case RespType::EMPTY:cout<<"EMPTY\n";
                            break;
                        case RespType::ARRAY:cout<<"Array\n";
                            break;
                        case RespType::BULK_STRING:cout<<"BULK_STRING\n";
                            break;
                        case RespType::SIMPLE_STRING :cout<<"SIMPLE_STRING\n";
                            break;
                            case RespType::INTEGER:cout<<"INTEGER\n";
                                break;
                        case RespType::ERROR:cout<<"ERROR\n";
                            break;
                        case RespType::INVALID:cout<<"INVALID\n";
                            break;
                        
                        default:
                            break;
                    }
                    user.inputBuffer="";

                }
                else if(bytes==0)
                {
                    cout<<fd<<" Connection Closed By Client\n";
                    close(fd);
                    epoll_event ev;
                    ev.events=EPOLLIN;
                    ev.data.fd=fd;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
                   
                    hash.erase(fd);
                }
                else
                {
                    if(errno!=EAGAIN)
                    {
                        cout<<strerror(errno)<<" "<<strerror(EAGAIN)<<" "<<fd<<endl;
                        epoll_event ev;
                        ev.events=EPOLLIN;
                        ev.data.fd=fd;
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
                        close(fd);
                        hash.erase(fd);
                    }
                }

            }
        }
    }
    return 0;
}