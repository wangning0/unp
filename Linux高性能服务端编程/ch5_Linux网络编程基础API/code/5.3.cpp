#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printf("usage: %s ip\n", basename(argv[0]));
        return 1;
    }
    struct hostent* addrInfo;
    struct in_addr addr;
    char** pptr;
    if(inet_pton(AF_INET, argv[1], &addr) <= 0)
    {
        printf("inet_pton error");
        return -1;
    }
    addrInfo = gethostbyaddr((const char*)&addr, sizeof(addr), AF_INET);
    if(addrInfo == NULL)
    {
        printf("addrInfo error");
        return 1;
    }
    printf("host name : %s\n", addrInfo->h_name);
    printf("host name : %d\n", addrInfo->h_addrtype);
    for(pptr = addrInfo->h_aliases; *pptr != NULL; pptr++)
    {
        printf("alias: %s\n", *pptr);
    }
    printf("host name : %d\n", addrInfo->h_length);
    return 0;
}