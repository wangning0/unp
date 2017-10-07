# Linux网络编程基础API

将从三个方面讨论Linux网络API
* socket地址API。socket最开始的含义是一个IP地址和端口对，它唯一地表示了使用TCP通信的一端。
* socket基础API。socket的主要API都定义在sys/socket.h头文件中，包括创建socket、明明socket、监听socket、接受连接、发起连接、读写数据、获取地址信息、检测带外标记以及读取和设置socket选项
* 网络信息API。Linux提供了一套网络信息API，以实现主机名和IP地址之间的转换，以及服务名称和端口号之间的转换，API定义在netdb.h头文件中

## socket地址API

要学习socket地址API，先要理解主机字节序和网络字节序

### 主机字节序和网络字节序

现代CPU的字节序分为大端和小端，大端的意思式高位字节存储在内存的低地址，低位字节存储在内存的高地址。小端指的是高位字节存储在内存的高地址，低位字节存储在内存的低地址

现代PC大多采用小端字节序，因为小端字节序又被称为主机字节序

当格式化的数据在两台使用不同字节序的主机之间直接传递时，接收端必然错误的解释。**解决的办法事：发送端总是把要打送的数据转化成大端字节序数据后再发送，而接收端知道对方传送过来的数据总是采用大端字节序，所以接收端可以根据自身采用的字节序决定是否对接收到的数据进行转换**

因此大段字节序也称为网络字节序，他给所有接收数据的主机提供了一个正确解释受到的格式化数据的保证

Linux提供了如下4个函数来完成主机字节序和网络字节序之间的转化

```
#include <netinet/in.h>
unsigned long int htonl(unsigned long int hostlong)
unsigned short int htons(unsigned short int hostshort)
unsigned long int ntohl(unsigned long int netlong)
unsigned short int ntohs(unsigned short int netshort)
```

`htonl` 表示 "host to network long" 即将长整型的主机字节序数据转化为网络字节序数据，这4个函数中，长整形函数通常用来转换IP地址，短整型函数用来转换端口号


### 通用socket地址

socket网络编程接口中表示socket地址的事结构体sockaddr

```
#include<bits/socket.h>
struct sockaddr
{
    sa_family_t sa_family;
    char sa_data[14];
}
```
sa_family成员事地址族类型 sa_family_t的变量，地址族类型通常与协议族类型对应。常见的协议族和对应的地址族对应关系如下

协议族 | 地址族 | 描述
--------- | ------------- | -----------
PF_UNIX | AF_UNIX | UNIX本地域协议族
PF_INET | AF_INET | TCP／IPv4协议族
PF_INET6 | AF_INET6 | TCP/IPv6协议族

宏 `PF_*` 和 `AF_*`都定义在`bits/socket.h`头文件中，且后者与前者都有完全相同的值，所以二者通常混用

sa_data成员用于存放socket地址值，但是，不同协议族的地址值具有不同的含义和长度

协议族 | 地址值含义和长度
------- | -----------
PF_UNIX | 文件的路径名，长度可达到108字节
PF_INET | 16bit端口号和32bitIPv4地址 共6字节
PF_INET6 | 16bit端口号，32bit流标识 128位IPv6地址，32位范围ID 共26字节

所以 Linux定义了下面这个新的通用socket地址结构体

```
#include <bits/socket.h>

struct sockaddr_storage 
{
    sa_family_t sa_family;
    unsigned long int __ss_align;
    char __ss_padding[128-sizeof(__ss_slign)]
}
// __ss_align成员的作用是内存对齐的
```

### 专用socket地址

Linux为各个协议族提供了专门的socket地址结构体

UNIX本地域协议族使用如下专用socket地址结构体

```
#include <sys/un.h>
struct sockaddr_un
{
    sa_family_t sin_family; // 地址族 AF_UNIX
    char sun_path[108]; // 文件路径名
}
```

TCP/IP协议族有sockaddr_in和sockaddr_in6两个专用socket地址结构体，分别用于IPv4和IPv6

```
strcut sockaddr_in
{
    sa_family_t sin_family; // AF_INET
    u_int16_t sin_port;  // 端口号，要用网络字节序表示
    struct in_addr sin_addr; // IPv4地址结构体
}
struct in_addr
{
    u_int32_t s_addr; // IPv4地址 要用网络字节序表示
}

struct sockaddr_in6
{
    sa_family_t sin6_family; // AF_INET6
    u_int16_t sin6_port; // 端口号 网络字节序表示
    u_int32_t sint_flowinfo; // 流信息 设置为0
    struct in6_addr sin6_addr; // IPv6地址结构体 
    u_int32_t sin6_scope_id; // scope ID 
}

struct in6_addr 
{
    unsigned char sa_addr[16]; //IPv6地址，网络字节序表示
}
```

### IP地址转换函数

人们习惯用可读性好的字符串来表示IP地址，比如用点分十进制字符串表示IPv4地址，以及用十六进制字符串表示IPv6地址。

但是编程中我们需要把它们转换成整数／二进制数 方能使用，而记录日志的时候，相反，把整数表示的IP地址转化为刻度的字符串

```
#include <arpa/inet.h>
in_addr_t inet_addr(const char* strptr)
int inet_aton(const char *cp, struct in_addr* inp)
char* inet_ntoa(struct in_addr in)
```

`inet_addr`函数将用点分十进制字符串表示的IPv4地址转化为用网络字节序证书表示的IPv4地址，失败时返INADDR_NONE

`inet_aton`函数完成和`inet_addr`同样的功能，但是将转化结果存储与参数inp指向的地址结构中，成功返回1，失败返回0

`inet_ntoa`函数将用网络字节序证书表示的IPv4地址转化为用点分十进制字符串表示的IPv4地址，但注意的事，该函数内部用一个静态变量存储转化结果，函数的返回值指向该静态内存，因此`inet_ntoa`是不可重入的

下面这对更新的函数也能完成和前面3个函数一样的功能

```
#include <arpa/inet.h>
int inet_pton(int af, const char* src, void* dst)
const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt)
```

`inet_pton`函数将用字符串表示的IP地址src转换成用网络字节序证书表示的OP地址，并把转换结果存储与dst指向的内存中，af表示的是地址族 `AF_INET/AF_INET6` 成功返回1，失败返回0

`inet_ntop`函数进行相反的转换，前三个参数的含义与`inet_pton`相同，最后一个参数cnt指定目标存储单元的大小

```
#include <netinet/in.h>
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
```

成功返回目标存储单元的地址，失败返回NULL

## 创建socket




