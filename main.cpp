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
struct ParseArrayHeader
{
    ParseStatus status;
    int arrayLength;
    int bytesConsumed;


};
struct ParseBulkString
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

ParseArrayHeader parseHeader(string &s)
{
    ParseArrayHeader res;
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
ParseBulkString parseString(string &s)
{
    ParseArrayHeader isArray=parseHeader(s);
    ParseBulkString res;
    if(isArray.status!=ParseStatus::OK)
    {
        
        res.status=isArray.status;
        return res;
    }
    cout<<"Parsing Bulk String: "<<s<<endl;
    int i=isArray.bytesConsumed;
    if(i>=s.size() || i+1>=s.size())
    {
    
        res.status=ParseStatus::NEED_MORE_DATA;
        return res;
    }
    if(s[i]!='$' || !(s[i+1]>='1' && s[i+1]<='9'))
    {
        cout<<"No number after $ sign: "<<i<<" "<<s[i]<<" "<<s[i+1]<<endl;
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

int main()
{
    string s="*2\r\n$3\r\nSET\r\n$5\r\nmykey\r\n";
    ParseArrayHeader res=parseHeader(s);
    if(res.status==ParseStatus::OK)
    {
        cout<<"Array Length : "<<res.arrayLength<<endl;
        cout<<"BytesConsumed : "<<res.bytesConsumed<<endl;
        auto bsres=parseString(s);
        if(bsres.status==ParseStatus::OK)
        {
            cout<<"Bulk String : "<<bsres.bulkString<<endl;
            cout<<"Bytes : "<<bsres.bytesConsumed<<endl;    
        }
        else if(bsres.status==ParseStatus::NEED_MORE_DATA)
        {
            cout<<"Need More Data\n";
        }
        else{
            cout<<"Formate Error\n";
        }
    }
}