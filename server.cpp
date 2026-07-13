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
    int arrayLength;
    vector<string> argv;
    Client()
    {
        
    }
    Client(int fd)
    {
        this->fd=fd;
        this->bytesConsumed=0;
        this->arrayLength=0;
    }
    
};

class Database{
    
    unordered_map<string,string> kv;
    public:
    Database(){}

    void set(string key,string val)
    {
        kv[key]=val;
    }

    string get(string key)
    {
        if(kv.find(key)==kv.end())
        return "";
        return kv[key];
    }
    void del(string key)
    {
        kv.erase(key);
    }


};

RespType getRespType(string &inputBuffer,int offset=0)
{
    if(inputBuffer.size()==0)
    return RespType::EMPTY;
    
    if(inputBuffer[offset]=='*')
    return RespType::ARRAY;
    
    if(inputBuffer[offset]=='$')
    return RespType::BULK_STRING;
    
    if(inputBuffer[offset]=='+')
    return RespType::SIMPLE_STRING;
    
    if(inputBuffer[offset]==':')
    return RespType::INTEGER;
    
    if(inputBuffer[offset]=='-')
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
    
 
    int i=offset;
    if(i>=s.size() || i+1>=s.size())
    {
     
     res.status=ParseStatus::NEED_MORE_DATA;
     return res;
    }

    if(getRespType(s,offset)!=RespType::BULK_STRING)
    {
        
        res.status=ParseStatus::ERROR;
        return res;
    }

    if(s[i]!='$' || !(s[i+1]>='0' && s[i+1]<='9'))
    {
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
    
    

    auto closeClientConnection=[&](int fd){
                        close(fd);
                        epoll_event ev;
                        ev.events=EPOLLIN;
                        ev.data.fd=fd;
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,&ev);
                        hash.erase(fd);
    };

    auto parser=[&](string &userInput,Client &user){

        ParseStatus res;
        if(user.arrayLength==0)
        {
           ArrayHeaderResult headerRes=parseHeader(userInput);
            if(headerRes.status==ParseStatus::OK)
            {
                user.arrayLength=headerRes.arrayLength;
                user.bytesConsumed=headerRes.bytesConsumed;
            }
            else if(headerRes.status==ParseStatus::NEED_MORE_DATA)
            {
                res=ParseStatus::NEED_MORE_DATA;
                return res;
            }
            else{
                res=ParseStatus::ERROR;
                return res;
            }
        }
        bool isError=false;
        while(user.arrayLength>0)
        {
            BulkStringResult bstr=parseString(user.inputBuffer,user.bytesConsumed);
            if(bstr.status==ParseStatus::OK)
            {
                user.argv.push_back(bstr.bulkString);
                user.bytesConsumed=bstr.bytesConsumed;
                user.arrayLength--;
            }
            else if(bstr.status==ParseStatus::NEED_MORE_DATA)
            {
                res=ParseStatus::NEED_MORE_DATA;
                return res;

            }
            else{
                isError=true;
                cout<<"Formate Error\n";
                res=ParseStatus::ERROR;
                return res;
            }
        }
        res=ParseStatus::OK;
        return res;

    };

    auto execute=[&](Client &user)
    {
            for(auto cmd:user.argv)
            cout<<cmd<<" ";
            cout<<endl;
            user.inputBuffer=user.inputBuffer.erase(0,user.bytesConsumed);
            user.bytesConsumed=0;
            user.argv.clear();

    };

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
                    
                    ParseStatus res=parser(user.inputBuffer,user);
                    if(res==ParseStatus::OK)
                    {
                        execute(user);
                        
                    }
                    else if(res==ParseStatus::NEED_MORE_DATA)
                    {
                        cout<<"Need More Data\n";
                        continue;

                    }
                    else 
                    {
                        cout<<"Formate Error\n";
                        closeClientConnection(fd);
                    }
                    
                    

                }
                else if(bytes==0)
                {
                    cout<<fd<<" Connection Closed By Client\n";
                    closeClientConnection(fd);
                }
                else
                {
                    if(errno!=EAGAIN)
                    {
                        cout<<strerror(errno)<<" "<<strerror(EAGAIN)<<" "<<fd<<endl;
                        closeClientConnection(fd);
                    }
                }


            }
        }
    }
    return 0;
}