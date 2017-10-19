#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10
#define MAX_FDS 5

void setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

void addfd(int epollfd, int listenfd, bool enable_et)
{
    epoll_event event;
    event.data.fd = listenfd;
    event.events = EPOLLIN;
    if(enable_et)
    {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);
    // 将文件描述符设置成非阻塞的
    setnonblocking(listenfd);
}

void lt(epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for(int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_addrlength);
            addfd(epollfd, connfd, false);
        }
        else  if(events[i].events & EPOLLIN)
        {
            // 只要socket读缓冲中还有未读出的数据，就会被触发以下代码
            printf("event trigger once\n");
            memset(buf, '\0', BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if(ret <= 0)
            {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content: %s\n", ret, buf);
        }
        else
        {
            printf("something else happened\n");
        }
    }
}

void et(epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for(int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_addrlength);

            addfd(epollfd, connfd, true);
        }
        else if(events[i].events & EPOLLIN)
        {
            // ET模式下这段代码不会重复触发，所以我们循环读取数据，以确保把socket读缓存中的所有数据读出
            printf("event trigger once\n");
            while(1)
            {
                memset(buf, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(ret < 0)
                {
                    // 对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕，此后epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下次读操作
                    if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        printf("read later\n");
                        break;
                    }
                    close(sockfd);
                    break;
                }
                else if(ret == 0)
                {
                    close(sockfd);
                }
                else
                {
                    printf("get %d bytes of content : %s\n", ret, buf);
                }
            }
        } 
        else 
        {
            printf("something else happened");
        }
    }
}

int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(MAX_FDS);
    assert(epollfd != -1);
    printf("-------");
    addfd(epollfd, sock, true);

    while(1)
    {
        /*
        int epoll_wait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout);*/
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0)
        {
            printf("epoll fail\n");
            break;
        }
        // lt模式
        // lt(events, ret, epollfd, sock);
        // et模式
        et(events, ret, epollfd, sock);
    }
    close(sock);
    return 0;
}