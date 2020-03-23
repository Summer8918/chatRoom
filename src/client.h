#ifndef CLIENT_H
#define CLIENT_H
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<string>
#include <thread>
#include<iostream>
using namespace std;

namespace chatRoom{
    class chatRoomClient{
    public:
        static const int BUF_SIZE = 0xFFFF;
        const char* SERVER_IP ="127.0.0.1";
        const int SERVER_PORT=8888;
        //epoll size
        const int EPOLL_SIZE = 5000;  
        chatRoomClient();
        ~chatRoomClient();
        void startSock();
        void onMessage();
        void addfd(int epollfd,int fd,bool enable_et);
        void setnonblocking(int sockfd);
        void threadFunc(int pipefd1,char *msg);
    private:
        struct sockaddr_in serverAddr;
        char message[BUF_SIZE];
        int pipe_fd[2];
        int epfd;
        int sockfd;
        //表示客户端是否正常工作
        static bool isClientwork;
        struct epoll_event events[2];
    };
}
#endif //CLIENT_H
