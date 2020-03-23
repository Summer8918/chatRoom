#include"client.h"
using namespace chatRoom;
//构造函数
chatRoomClient::chatRoomClient(){
    this->startSock();
    //创建管道,其中fd[0]用于父进程读，fd[1]用于子进程写
    if(pipe(pipe_fd)<0){
        perror("pipe error");
        exit(-1);
    }
    epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("epfd error");
        exit(-1);
    }
    //将sock和管道读端描述符添加到内核事件表中
    addfd(epfd,sockfd,true);
    addfd(epfd,pipe_fd[0],true);
    isClientwork=true;
    std::cout<<isClientwork<<std::endl;
}

chatRoomClient::~chatRoomClient(){
    //关闭socket
    close(sockfd);
    //关闭epoll
    close(epfd);
}
void chatRoomClient::startSock(){
    //用户连接的服务器 IP + port
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    //创建socket
    sockfd = socket(PF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        perror("sock error");
        exit(-1);
    }
    //连接服务端
    if(connect(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
        perror("connect error");
        exit(-1);
    }
}

void chatRoomClient::addfd(int epollfd,int fd,bool enable_et){
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

/*
 * @para sockfd:socket descriptor
 * @return 0*/
void chatRoomClient::setnonblocking(int sockfd){
    fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFD,0) | O_NONBLOCK);
}

void chatRoomClient::onMessage(){
    //fork
    pid_t pid = fork();
    if(pid < 0){
        perror("fork error");
        exit(-1);
    }
    else if(pid == 0){//子进程
        //子进程负责写入管道，因此先关闭读端
        close(pipe_fd[0]);
        printf("please input 'exit' to exit the chat room\n");
        while(isClientwork){    //待解决的问题 isClientwork的同步
            bzero(&message,BUF_SIZE);
            fgets(message,BUF_SIZE,stdin);
            //printf("%s\n",message);
            //cout<<"son process echo"<<endl;
            //客户输入exit,退出
            if(strncasecmp(message,"EXIT",strlen("EXIT"))==0){
                isClientwork=0;
            }
            //子进程将信息写入管道
            else{
                if(write(pipe_fd[1],message,strlen(message)-1) < 0){
                    perror("fork error");
                    exit(-1);
                }
                printf("son process write to pipe ok\n");
            }
        }
    }
    else{//pid>0 父进程
        //父进程负责读管道数据，因此先关闭写端
        close(pipe_fd[1]);
    
        //主循环(epoll_wait)
        printf("this is patent process\n");
        while(isClientwork){
            int epoll_events_count = epoll_wait(epfd,events,2,-1);
            //处理就绪事件
            for(int i=0;i<epoll_events_count;++i){
                //服务器端发来消息
                if(events[i].data.fd == sockfd){
                    //接收服务端信息
                    char message2[BUF_SIZE];
                    int ret = recv(sockfd,message2,BUF_SIZE,0);
                    //ret = 0服务端关闭
                    if(ret==0){
                        printf("Server closed connection:%d\n",sockfd);
                        close(sockfd);
                        isClientwork=false;
                    }
                    else{
                        printf("ret=%d message from server\n",ret);
                        printf("%s\n",message2);
                    }
                }
                //子进程写入事件发生，父进程处理并发送服务端
                else{
                    bzero(&message,BUF_SIZE);
                    //父进程从管道中读取数据
                    int ret = read(events[i].data.fd,message,BUF_SIZE);
                    printf("read message from son process :%s\n",message);
                    //ret = 0
                    if(ret==0){
                        isClientwork = 0;
                    }
                    //将信息发送给服务器
                    else{
                        send(sockfd,message,BUF_SIZE,0);
                        printf("send message to server\n");
                    }
                }
            }
        }
    }
    if(pid){
        //关闭父进程和sock 
        close(pipe_fd[0]);
        close(sockfd);
        printf("wait for son process stop\n");
        if(waitpid(pid,NULL,0)!=pid){
            perror("waitpid error");
        }
        exit(0);
    }
    else{
        //关闭子进程
        close(pipe_fd[1]);
    }
}