#include "utility.h"

int main(){
    //server ip + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    //create listen socket
    int listener = socket(PF_INET,SOCK_STREAM,0);
    if(listener <0){
        perror("listener error\n");
        exit(-1);
    }
    printf("listen socket created");
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

    //create event table in kernal
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("create epoll fd error");
        exit(-1);
    }
    printf("epoll created, epollfd = %d\n",epfd);
    static struct epoll_event events[EPOLL_SIZE];
    //add events to kernel event table
    addfd(epfd,listener,true);

    //main while
    while(1){
        //epoll_events_counts stand for ready evnets numbers
        int epoll_events_count = epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if(epoll_events_count < 0){
            perror("epoll failure");
            break;
        }

        printf("epoll_events_count = %d\n",epoll_events_count);
        //处理epoll_events_count个就绪事件
        for(int i=0;i<epoll_events_count;++i){
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener){
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
            //处理用户发来的消息,并广播,使其他用户收到信息
            else{
                int ret = sendBroadcastmessage(sockfd);
                if(ret < 0){
                    perror("error");
                    exit(-1);
                }
            }
        }
    }
    //关闭socket
    close(listener);
    //关闭epoll
    close(epfd);
}
