#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        printf("Usage: %s ip_address port_number\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(sock, (struct sockaddr*)&client_address, &client_addrlength);
    
    if(connfd < 0)
    {
        printf("errno is %d\n", errno);
        close(connfd);
    }
    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while(1)
    {
        memset(buf, '\0', sizeof(buf));
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);
        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);
        
        if(ret < 0)
        {
            printf("select error\n");
            break;
        }
        if(FD_ISSET(connfd, &read_fds))
        {
            ret = recv(connfd, buf, sizeof(buf) - 1, 0);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes pf normal data: %s\n", ret, buf);
        }
        else if(FD_ISSET(connfd, &exception_fds)) 
        {
            ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes pf oob data: %s\n", ret, buf);
        }
    }
    close(connfd);
    close(sock);
    return 0;
}