#include <sys/sem.h>
#include <stdio.h>
#define MYKEY 666

int main()
{
    int semid;
    semid = semget(MYKEY, 100, 0666 | IPC_CREAT);
    printf("semid: %d", semid);
    return 0;
}

/**
 * ipcs -s 查看信号量
 * 
*/