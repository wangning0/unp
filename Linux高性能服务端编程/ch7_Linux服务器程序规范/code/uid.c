#include <unistd.h>
#include <stdio.h>

int main()
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf("uid: %d, euid: %d\n", uid, euid);
    return 0;
}

/**
 * 
 * sudo chown root:root ./uid
 * sudo chmod +s ./uid
*/