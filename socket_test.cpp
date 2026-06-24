#include<iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<cerrno>
#include<fcntl.h>
#include<cstring>

using namespace std;

int main()
{
    int socketFd=socket(AF_INET,SOCK_STREAM,0);
    if(socketFd<0)
    {
        cout<<"Socket Creation Failed\n";
        return 0;
    }
    sockaddr_in socketAddr;
    socketAddr.sin_family=AF_INET;
    socketAddr.sin_port=htons(8080);
    socketAddr.sin_addr.s_addr=INADDR_ANY;

    if(bind(socketFd,(struct sockaddr*)&socketAddr,sizeof(socketAddr))<0)
    {
        cout<<"Binding Failed\n";
        return 0;
    }
    int fl=fcntl(socketFd,F_GETFL);
    fl|=O_NONBLOCK;
    fcntl(socketFd,F_SETFL,fl);
    cout<<"Binding Success and now listening in non-blocking mode\n";
    listen(socketFd,5);
    int clientFd=-1;
    while(clientFd<0)
    {
        clientFd=accept(socketFd,nullptr,nullptr);
        cout<<strerror(errno)<<"\n";
        sleep(1);
    }
    cout<<"Client Connected with fd : "<<clientFd<<" Errno is :"<<strerror(errno)<<endl;
    fl=fcntl(clientFd,F_GETFL);
    fl|=O_NONBLOCK;
    fcntl(clientFd,F_SETFL,fl);

    char buffer[1028]={0};
    bool flag=true;
    while(flag)
    {
        int n=recv(clientFd,buffer,sizeof(buffer),0);
        if(n<0)
        {
            cout<<"Errno is : "<<strerror(errno);
            sleep(1);
        }
        
        if(strcmp(buffer,"exit\n")==0)
        flag=false;
        cout<<buffer;
    }
    
    close(socketFd);
    return 0;
}