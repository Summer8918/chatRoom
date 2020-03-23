#ifndef SERVER_H
#define SERVER_H
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
#include<list>
#include<string>
#include <thread>
#include<iostream>
namespace chatRoom{
    #define SERVER_WELCOME "Welcome you join to the chat room! Your char ID is:Client #%d"
    #define SERVER_MESSAGE "ClientID %d say >> %s"
    class chatRoomServer{
    public:
        chatRoomServer();
        ~chatRoomServer();
        void sockStart(int &listener);
        void loop();
    private:
        static const int BUF_SIZE = 0xFFFF;
        const char* SERVER_IP ="127.0.0.1";
        const char* CAUTION = "There is only one in the chat room!";
        const int SERVER_PORT=8888;
        //epoll size
        static const int EPOLL_SIZE = 5000;  
        int epfd;
        int listener;  //listen socket
        struct epoll_event events[EPOLL_SIZE];  //是不是太大了？
        //client_list save all the clients's socket
        std::list<int> clients_list;
        void addfd(int epollfd,int fd,bool enable_et);
        void setnonblocking(int sockfd);
        void handleNewConnection();
        int sendBroadcastmessage(int clientfd);
    };
}
#endif