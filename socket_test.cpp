
#include<iostream>
#include<cstring>
#include<unistd.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/epoll.h>
using namespace std;

void makeNonBlock(int fd)
{
    if(fd<0)
    return ;

    int fl=fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,fl|O_NONBLOCK);
}

int main()
{
    int socketFd=socket(AF_INET,SOCK_STREAM,0);
    if(socketFd<0)
    {
        cout<<strerror(errno)<<endl;
        return 0;
    }
    cout<<"Socket Created Successfully\n";

    sockaddr_in socketAddr;
    socketAddr.sin_family=AF_INET;
    socketAddr.sin_port=htons(8080);
    socketAddr.sin_addr.s_addr=INADDR_ANY;

    if(bind(socketFd,(sockaddr*)&socketAddr,sizeof(socketAddr))<0)
    {
        cout<<strerror(errno)<<endl;
        close(socketFd);
        return 0;
    }
    cout<<"Socket Binding success\n";
    makeNonBlock(socketFd);
    constexpr int MAX_N=64;
    listen(socketFd,MAX_N);
    cout<<"Listenining...\n";

    int epfd=epoll_create1(0);
    if(epfd<0)
    {
        cout<<strerror(errno)<<endl;
        close(socketFd);
        return 0;
    }
    epoll_event ev;
    ev.events=EPOLLIN;
    ev.data.fd=socketFd;

    if(epoll_ctl(epfd,EPOLL_CTL_ADD,socketFd,&ev)<0)
    {
        cout<<strerror(errno)<<endl;
        close(socketFd);
        close(epfd);
        return 0;
    }

    epoll_event events[MAX_N];
    char buffer[1028]={0};
    
    while(1)
    {
        int n=epoll_wait(epfd,events,MAX_N,-1);

        for(int i=0;i<n;i++)
        {
            int fd=events[i].data.fd;
            if(fd==socketFd)
            {
                int clientFd=0;

                while(1)
                {
                    clientFd=accept(socketFd,nullptr,nullptr);

                    if(clientFd<0)
                    break;

                    makeNonBlock(clientFd);
                    epoll_event ev;
                    ev.events=EPOLLIN;
                    ev.data.fd=clientFd;

                    if(epoll_ctl(epfd,EPOLL_CTL_ADD,clientFd,&ev)<0)
                    {
                        cout<<strerror(errno)<<endl;
                        close(clientFd);
                        continue;
                    }
                }
            }
            else{
                int bytesRead=recv(fd,buffer,sizeof(buffer),0);
                if(bytesRead>0)
                {
                    while(bytesRead>0)
                    {
                        cout<<buffer;
                        memset(buffer,0,sizeof(buffer));
                        bytesRead=recv(fd,buffer,sizeof(buffer),0);
                    }

                }
                else if(bytesRead==0 || (errno != EAGAIN)){
                    cout<<strerror(errno)<<" "<<strerror(EAGAIN)<<" "<<fd<<endl;
                    epoll_event ev;
                    ev.events=EPOLLIN;
                    ev.data.fd=fd;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
                    close(fd);
                }
            }
        }
    }
    close(socketFd);
    close(epfd);

    return 0;
}
