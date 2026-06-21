#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

using namespace std;

int main()
{
    int serverFd=socket(AF_INET,SOCK_STREAM,0);
    if(serverFd<0)
    {
        cout<<"Erro occured\n";
        return 0;
    }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(serverFd,(struct sockaddr*)&serverAddress,sizeof(serverAddress))<0)
    {
        cout<<"Binding failed\n";
        return 0;

    }

    listen(serverFd,5);
    int clientFd=accept(serverFd,nullptr,nullptr);
    if(clientFd<0)
    {
        cout<<"Connection failed\n";
        return 0;
    }
    char buffer[1024]={0};
    while(1)
    {
        recv(clientFd,buffer,sizeof(buffer),0);
        cout<<"Message from client: "<<buffer<<endl;
    }
    close(serverFd);

    return 0;
}
