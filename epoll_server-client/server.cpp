#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS "127.0.0.1"
#define PORT 8787
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 100

// 函数声明
// 创建套接字并进行绑定
static int socket_bind(const char* ip, int port);
// IO多路复用epoll
static void do_epoll(int listenfd);
// 事件处理函数
static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char* buf);
// 处理接收到的连接
static void handle_accept(int epollfd, int listenfd);
// 读处理
static void do_read(int epollfd, int fd, char* buf);
// 写处理
static void do_write(int epollfd, int fd, char* buf);
// 添加事件
static void add_event(int epollfd, int fd, int state);
// 修改事件
static void modify_event(int epollfd, int fd, int state);
// 删除事件
static void delete_event(int epollfd, int fd, int state);

int main()
{
    int listenfd;
    listenfd = socket_bind(IPADDRESS, PORT);
    listen(listenfd, LISTENQ);
    do_epoll(listenfd);
    return 0;
}

static int socket_bind(const char* ip, int port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1) 
    {
        perror("socket error: ");
        return 1;
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        return 1;
    }
    return listenfd;
}

static void do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf, 0, MAXSIZE);
    // 创建一个描述符
    epollfd = epoll_create(FDSIZE);
    // 添加监听描述符事件
    add_event(epollfd, listenfd, EPOLLIN); // 表示对应的文件描述符可以读
    for(;;)
    {
        // 获取已经准备好的事件, 返回值表示的是处理的事件数目 -1参数表示的是不确定超时时间
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        handle_events(epollfd, events, ret, listenfd, buf);
    }
    close(epollfd);
}

static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char* buf)
{
    int i, fd;
    for(i = 0; i < num; i++)
    {
        fd = events[i].data.fd;
        if((fd == listenfd) && (events[i].events & EPOLLIN))
            handle_accept(epollfd, listenfd);
        else if (events[i].events & EPOLLIN)
            do_read(epollfd, fd, buf);
        else if(events[i].events & EPOLLOUT)
            do_write(epollfd, fd, buf);
    }
}

static void handle_accept(int epollfd, int listenfd)
{
    int clientfd;
    struct sockaddr_in clientaddr;
    socklen_t client_addr_length;
    clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &client_addr_length);
    if(clientfd == -1)
    {
        perror("accept error: ");
    }
    else
    {
        printf("accept a new client: %s:%d\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
        add_event(epollfd, clientfd, EPOLLIN);
    }
}

static void do_read(int epollfd, int fd, char* buf)
{
    int nread;
    nread = read(fd, buf, MAXSIZE);
    if(nread == -1)
    {
        perror("read error: ");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else if(nread == 0)
    {
        fprintf(stderr, "client close.\n");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else
    {
        printf("read message is : %s", buf);
        modify_event(epollfd, fd, EPOLLOUT);
    }
}

static void do_write(int epollfd, int fd, char* buf)
{
    int nwrite;
    nwrite = write(fd, buf, strlen(buf));
    if (nwrite == -1)
    {
        perror("write error: ");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else
    {
        modify_event(epollfd, fd, EPOLLIN);
    }
    memset(buf, 0, MAXSIZE);
}

static void add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

static void modify_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

static void delete_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}