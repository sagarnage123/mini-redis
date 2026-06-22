#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include<cstring>
#include<fcntl.h>
using namespace std;



int main()
{
    int socketId=socket(AF_INET,SOCK_STREAM,0);
    if(socketId<0)
    {
        cout<<"Socket Creation failed\n";
        return 0;
    }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if(bind(socketId,(struct sockaddr*)&serverAddress,sizeof(serverAddress))<0)
    {
        cout<<"Binding failed\n";
        return 0;
    }

    cout<<"Listeining\n";
    int fl=fcntl(socketId,F_GETFL);
    fl|=O_NONBLOCK;
    fcntl(socketId,F_SETFL,fl);

    listen(socketId,5);

    int clientId=accept(socketId,nullptr,nullptr);
    int saftey=10000

    while(clientId<0 && saftey>0)
    {
        cout<<"Connect the client\n"<<"Returned value : "<<clientId<<endl;
        clientId=accept(socketId,nullptr,nullptr);
        saftey--;
    }
    cout<<"Client Connceted\n";
    char buffer[1028]={0};
    bool flag=true;
    cout<<"Send message if you want to or press exit to stop\n";
    while(flag)
    {
        memset(buffer,0,sizeof(buffer));
        int bytesReceived = recv(clientId,buffer,sizeof(buffer),0);

        if(bytesReceived<0)
        {
            cout<<"Receiving failed\n";
            close(socketId);
            return 0;
        }

        if(strcmp(buffer,"exit\n")==0)
        flag=false;
        cout<<buffer;
        
    }
    close(socketId);
    return 0;
}