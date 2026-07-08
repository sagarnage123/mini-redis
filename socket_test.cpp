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
enum class ParseStatus
{
    OK,
    NEED_MORE_DATA,
    ERROR

};
struct ArrayHeaderResult
{
    ParseStatus status;
    int arrayLength;
    int bytesConsumed;


};
struct BulkStringResult
{
    ParseStatus status;
    string bulkString;
    int bytesConsumed;
};
class Client
{
    public:
    int fd;
    string inputBuffer;
    int bytesConsumed;
    Client()
    {
        
    }
    Client(int fd)
    {
        this->fd=fd;
        this->bytesConsumed=0;
    }
    
};

RespType getRespType(string &inputBuffer)
{
    if(inputBuffer.size()==0)
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

ArrayHeaderResult parseHeader(string &s)
{
    ArrayHeaderResult res;
    if(getRespType(s) != RespType::ARRAY)
    {
        res.status=ParseStatus::ERROR;
        return res;
    }
    int cnt=0;
    for(int i=1;i<s.size()-1;i++)
    {
        if(s[i]=='\r' && s[i+1]=='\n')
        {
            res.status=ParseStatus::OK;
            res.bytesConsumed=i+2;
            res.arrayLength=cnt;
            return res;
        }
        if(s[i]>='0' && s[i]<='9')
        {
            cnt=cnt*10+(s[i]-'0');
        }
        else{
            
            res.status=ParseStatus::ERROR;
            return res;
        }
    }
    res.status=ParseStatus::NEED_MORE_DATA;
    return res;
}
BulkStringResult parseString(string &s,int offset=0)
{
   
    BulkStringResult
 res;
    if(getRespType())
    {
        
        res.status=isArray.status;
        return res;
    }
   
    int i=isArray.bytesConsumed;
    if(i>=s.size() || i+1>=s.size())
    {
    
        res.status=ParseStatus::NEED_MORE_DATA;
        return res;
    }
    if(s[i]!='$' || !(s[i+1]>='1' && s[i+1]<='9'))
    {
        cout<<"No $ or invalid number after $ sign: "<<i<<" "<<s[i]<<" "<<s[i+1]<<endl;
        res.status=ParseStatus::ERROR;
        return res;
    }
    i++;
    int len=0;
    for(;i<s.size()-1;i++)
    {
        if(s[i]=='\r' && s[i+1]=='\n')
        {
            break;
        }
        if(s[i]>='0' && s[i]<='9')
        {
            len=len*10+(s[i]-'0');
        }
        else{
            cout<<"Invalid number after $ sign: "<<i<<" "<<s[i]<<endl;
            res.status=ParseStatus::ERROR;
            return res;
        }
    }
    
    i+=2;

    if(i+len+2>s.size())
    {
        res.status=ParseStatus::NEED_MORE_DATA;
        return res;
    }
   
    if(s[i+len]!='\r' || s[i+len+1]!='\n')
    {
        res.status=ParseStatus::ERROR;
        cout<<"Invalid bulk string format\n";
        return res;
    }
    for(int idx=i;idx<i+len;idx++)
    {
        res.bulkString+=s[idx];
    }
    res.bytesConsumed=i+len+2;
    res.status=ParseStatus::OK;
    return res;

}
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
                    
                    ArrayHeaderResult res=parseHeader(user.inputBuffer);
                    if(res.status==ParseStatus::OK)
                    {
                        cout<<"Array Length : "<<res.arrayLength<<endl;
                        cout<<"BytesConsumed : "<<res.bytesConsumed<<endl;
                        auto bsres=parseString(user.inputBuffer);
                        if(bsres.status==ParseStatus::OK)
                        {
                            cout<<"Bulk String : "<<bsres.bulkString<<endl;
                            cout<<"Bytes : "<<bsres.bytesConsumed<<endl;
                        }
                        else if(bsres.status==ParseStatus::NEED_MORE_DATA)
                        {
                            cout<<"Need More Data\n";
                            continue;

                        }
                        else{
                            cout<<"Formate Error\n";
                            close(fd);
                            epoll_event ev;
                            ev.events=EPOLLIN;
                            ev.data.fd=fd;
                            epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);

                        }
                    }
                    else if(res.status==ParseStatus::NEED_MORE_DATA)
                    {
                        cout<<"Need More Data\n";
                        continue;

                    }
                    else 
                    {
                        cout<<"Formate Error\n";
                        close(fd);
                        epoll_event ev;
                        ev.events=EPOLLIN;
                        ev.data.fd=fd;
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
                    }
                    
                    

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