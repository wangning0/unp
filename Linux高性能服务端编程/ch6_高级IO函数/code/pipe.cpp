#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
int main()
{
    int fd[2];
    int r = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if(r < 0)
    {
        perror("sockerpair error: ");
        return 1;
    }
    if(fork())
    {  
        // 父进程
        int val = 0;
        close(fd[1]);
        while(1)
        {
            sleep(1); // 睡1s
            ++val;
            printf("发送数据: %d\n", val);
            write(fd[0], &val, sizeof(val));
            read(fd[0], &val, sizeof(val));
            printf("收到收据：%d\n", val);
        }
    }
    else
    {
        int val;
        close(fd[0]);
        while(1)
        {
            read(fd[1], &val, sizeof(val));
            ++val;
            write(fd[1], &val, sizeof(val));
        }
    }
}
