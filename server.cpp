#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<fcntl.h>
#include<unistd.h>
#include<netinet/in.h>
#include<cerrno>
using namespace std;
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
        return 0;
    }
    cout<<"Socket Bind successfully\n";
    int fl=fcntl(socketFd,F_GETFL);
    fl|=O_NONBLOCK;
    fcntl(socketFd,F_SETFL,fl);

    listen(socketFd,5);
    cout<<"Listening .... \n";

    int clientFd=accept(socketFd,nullptr,nullptr);
    while(clientFd<0)
    {
        clientFd=accept(socketFd,nullptr,nullptr);
        cout<<strerror(errno)<<"\n";
        sleep(1);
    }
    fl=fcntl(clientFd,F_GETFL);
    fl|=O_NONBLOCK;
    fcntl(clientFd,F_SETFL,fl);
    cout<<"Client Connected Succseefully\n";

    char buffer[1028]={0};
    bool flag=true;

    while(flag)
    {
        memset(buffer,0,sizeof(buffer));
        recv(clientFd,buffer,sizeof(buffer),0);
        if(strcmp(buffer,"exit\n")==0)
        flag=false;
        cout<<buffer;
    }
    close(socketFd);

    return 0;
}