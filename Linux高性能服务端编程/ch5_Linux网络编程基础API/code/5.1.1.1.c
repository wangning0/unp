#include <stdio.h>

union var{
    char c[4];  
    int i; 
};

int main() {
    union var data;
    data.c[0] = 0x04;
    data.c[1] = 0x03;
    data.c[2] = 0x02;
    data.c[3] = 0x11;
    printf("%x\n", data.i);
    return 0;
}