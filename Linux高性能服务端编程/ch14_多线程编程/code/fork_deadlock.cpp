#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
// #include <wait.h>

pthread_mutex_t mutex;

void* another(void *arg)
{
    printf("in child thread, lock the mutex");
    pthread_mutex_lock(&mutex);
    sleep(5);
    pthread_mutex_unlock(&mutex);
    return (void*)0;
}

int main()
{
    pthread_mutex_init(&mutex, NULL);
    pthread_t id;
    pthread_create(&id, NULL, another, NULL);
    sleep(1);
    int pid = fork();
    if(pid < 0)
    {
        pthread_join(id, NULL);
        pthread_mutex_destroy(&mutex);
        return 1;
    }
    else if(pid == 0)
    {
        printf("child process");
        pthread_mutex_lock(&mutex);
        printf("can not run");
        pthread_mutex_unlock(&mutex);
        exit(0);
    }
    else
    {
        wait(NULL);
    }
}