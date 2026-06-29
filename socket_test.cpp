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
                    int idx=0;
                    string temp="";
                    for(int i=0;i<user.inputBuffer.size();i++)
                    {
                        char ch=user.inputBuffer[i];
                        if(ch=='\n')
                        {
                            //execution
                            cout<<temp<<endl;
                            temp="";
                            idx=i+1;
                            
                        }
                        else{
                            temp+=ch;
                        }

                    }
                    if(idx<user.inputBuffer.size())
                    user.inputBuffer=user.inputBuffer.substr(idx);
                    else user.inputBuffer="";

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