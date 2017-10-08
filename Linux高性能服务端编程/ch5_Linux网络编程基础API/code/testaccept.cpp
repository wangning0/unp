#include <sys/socket.h>
#include <netinet/in.h> // 目的是完成主机字节序和网络字节序之间的转化
#include <arpa/inet.h> // IP地址转换函数
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if(argc <= 2)
    {
        printf("Usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address)); // strings.h头文件
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    // 暂停20s已等待客户端连接和相关操作
    sleep(20); // 需要头文件 unistd
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if(connfd < 0) 
    {
        printf("errno is : %d\n", errno);
    }
    else
    {
        // 接受连接成功则打印出客户端的IP地址和端口号
        char remote[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN 是in.h里定义的宏
        printf("connected with ip : %s and port: %d\n", inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN), ntohs(client.sin_port));
        close(connfd);
    }
    close(sock);

    return 0;
}


