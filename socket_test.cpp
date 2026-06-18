#include <iostream>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>   

using namespace std;

int main()
{
    cout<<"Hello worlds\n";
    int id=socket(AF_INET,SOCK_STREAM,0);
    cout<<getpid()<<endl;
    if(id<0)
    {
        cout<<"Error occured\n";
    }
    else{
        cout<<"Socket opend succfully with id: "<<id<<endl;
        sleep(30);
        close(id);
        cout<<"Scoket closed\n";

        
    }
    return 0;
}