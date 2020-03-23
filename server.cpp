#include"server.h"
using namespace chatRoom;

chatRoomServer::~chatRoomServer(){
    //关闭socket
    close(listener);
    //关闭epoll
    close(epfd);
}

chatRoomServer::chatRoomServer(){
    sockStart(listener);
    //create event table in kernal
    chatRoomServer::epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("create epoll fd error");
        exit(-1);
    }
    printf("epoll created, epollfd = %d\n",epfd);
    //add events to kernel event table
    addfd(epfd,listener,true);
}

void chatRoomServer::addfd(int epollfd,int fd,bool enable_et){
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et){
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
    setnonblocking(fd);
    printf("fd added to epoll!\n");
}   

void chatRoomServer::handleNewConnection(){
    struct sockaddr_in client_address;
    socklen_t client_addrLength = sizeof(struct sockaddr_in);
    int clientfd = accept(listener,(struct sockaddr*)&client_address,&client_addrLength);
    printf("client connection from:%s : %d(IP:port),clientfd = %d \n",
                    inet_ntoa(client_address.sin_addr),
                    ntohs(client_address.sin_port),
                    clientfd);
    addfd(epfd,clientfd,true);

    //服务器端用clients_list保存用户连接
    clients_list.push_back(clientfd);
    printf("Add new clientfd = %d to epoll\n",clientfd);
    printf("Now there are %d clients in the chat room\n",(int)clients_list.size());

    //服务器端发送欢迎信息
    printf("welconme message\n");
    char message[BUF_SIZE];
    bzero(message,BUF_SIZE);
    sprintf(message,SERVER_WELCOME,clientfd);
    int ret = send(clientfd,message,BUF_SIZE,0);
    if(ret < 0){
        perror("send error");
        exit(-1);
    }
}

void chatRoomServer::loop(){
    while(1){
        //epoll_events_counts stand for ready evnets numbers
        int epoll_events_count = epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if(epoll_events_count < 0){
            perror("epoll failure");
            break;
        }
        printf("epoll_events_count = %d\n",epoll_events_count);
        for(int i=0;i<epoll_events_count;++i){
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener){
                handleNewConnection();
            }
            //处理用户发来的消息,并广播,使其他用户收到信息
            else{
                int ret = sendBroadcastmessage(sockfd);
                printf("ret=%d\n",ret);
                if(ret < 0){
                    perror("error");
                    exit(-1);
                }
            }
        }
    }
}
/*
 * @para sockfd:socket descriptor
 * @return 0
 * */
void chatRoomServer::setnonblocking(int sockfd){
    fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFD,0) | O_NONBLOCK);
}

void chatRoomServer::sockStart(int &listener){
    //server ip + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    //create listen socket
    listener = socket(PF_INET,SOCK_STREAM,0);
    if(listener <0){
        perror("listener error\n");
        exit(-1);
    }
    printf("listen socket created");
    //避免time_wait期间不能重用套接字
    int on=1;
    if((setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0){
        perror("setsockopt failed");
        exit(-1);
    }

    //bind address
    if(bind(listener,(struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
        perror("bind error\n");
        exit(-1);
    }

    //listen
    int ret = listen(listener,5);
    if(ret < 0){
        perror("listen error");
        exit(-1);
    }
    printf("start to listen:%s\n",SERVER_IP);
}

/*
* @para clientfd: socket descriptor
* @return : len
* */
int chatRoomServer::sendBroadcastmessage(int clientfd){
    //buf[BUF_SIZE] receive new chat message
    //message[BUF_SIZE] save format message
    char buf[BUF_SIZE],message[BUF_SIZE];
    bzero(buf,BUF_SIZE);
    bzero(message,BUF_SIZE);

    //receive message
    printf("read from client(clientID= %d)\n",clientfd);
    int len = recv(clientfd,buf,BUF_SIZE,0);
    printf("len=%d the message is: %s\n",len,buf);
    //means the client closed connection
    if(len==0){
        close(clientfd);
        //server remove the client
        clients_list.remove(clientfd);
        printf("ClientID = %d closed.\n now there are %d clients in the chat room\n",clientfd,(int)clients_list.size());
    }
    else{//broadcast message
        if(clients_list.size()==1){ //only one int the chat room
            send(clientfd,CAUTION,strlen(CAUTION),0);
        }
          //format message to broadcast
        sprintf(message,SERVER_MESSAGE,clientfd,buf);

        std::list<int>::iterator it;
        for(it = clients_list.begin();it != clients_list.end();++it){
            if(*it != clientfd){
                if(send(*it,message,BUF_SIZE,0)<0){
                    perror("error");
                    exit(-1);
                }
            }
        }
        return len;
    }
    //format message to broadcast
    sprintf(message,SERVER_MESSAGE,clientfd,buf);

    std::list<int>::iterator it;
    for(it = clients_list.begin();it != clients_list.end();++it){
        if(*it != clientfd){
            if(send(*it,message,BUF_SIZE,0)<0){
                perror("error");
                exit(-1);
            }
        }
    }
    return len;
}