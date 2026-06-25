#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<fcntl.h>
#include<unistd.h>
#include<netinet/in.h>
#include<cerrno>
#include<sys/epoll.h>
using namespace std;

void makeNonBlock(int fd)
{
     int fl=fcntl(fd,F_GETFL);
    fl|=O_NONBLOCK;
    fcntl(fd,F_SETFL,fl);

}
int main()
{
    int socketFd=socket(AF_INET,SOCK_STREAM,0);
    if(socketFd<0)
    {
        cout<<"Socket Creation Failed\n"<<strerror(errno)<<endl;
        return 0;
    }
    cout<<"Socket Created ...\n";
    sockaddr_in socketAddr;
    socketAddr.sin_family=AF_INET;
    socketAddr.sin_port=htons(8080);
    socketAddr.sin_addr.s_addr=INADDR_ANY;

    if(bind(socketFd,(struct sockaddr*)&socketAddr,sizeof(socketAddr))<0)
    {
        cout<<strerror(errno)<<endl;
         close(socketFd);
        return 0;
    }
    cout<<"Socket Bind successfully\n";

    makeNonBlock(socketFd);

    listen(socketFd,5);
    cout<<"Listening .... \n";

    int epfd=epoll_create1(0);
    struct epoll_event ev;
    ev.events=EPOLLIN;
    ev.data.fd=socketFd;

    if(epoll_ctl(epfd,EPOLL_CTL_ADD,socketFd,&ev)<0)
    {
        cout<<strerror(errno)<<endl;
        close(epfd);
        close(socketFd);
        return 0;
    }
    int n=64;
    struct epoll_event events[n];
    char buffer[1048]={0};
    while(true)
    {
        int len=epoll_wait(epfd,events,n,-1);

        for(int i=0;i<len;i++)
        {
            if(events[i].data.fd==socketFd)
            {
                int clientFd=accept(socketFd,nullptr,nullptr);
                if(clientFd<0)
                {
                    cout<<strerror(errno)<<endl;
                    continue;
                }
                makeNonBlock(clientFd);

                struct epoll_event ev;
                ev.events=EPOLLIN;
                ev.data.fd=clientFd;

                if(epoll_ctl(epfd,EPOLL_CTL_ADD,clientFd,&ev)<0)
                {
                    cout<<strerror(errno)<<endl;
                    continue;

                }
                cout<<endl<<"New Entry with Fd : "<<clientFd<<endl;
            }
            else{
                int clientFd=events[i].data.fd;
                memset(buffer,0,sizeof(buffer));

                int cur=recv(clientFd,buffer,sizeof(buffer),0);
                if(cur<=0)
                {
                    cout<<strerror(errno)<<endl;
                    close(epfd);
                    close(socketFd);
                    return 0;
                }
                else cout<<"Message from: "<<clientFd<<"->"<<buffer;
                
            }
        }

    }

    
    close(socketFd);
    close(epfd);

    return 0;
}