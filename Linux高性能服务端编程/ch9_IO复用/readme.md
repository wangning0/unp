# I/O复用

I/O复用使得程序能同时监听多个文件描述符，这对程序的性能至关重要。通常网络程序在下列情况下需要使用I／O复用技术

* 客户端要同时处理多个socket，本章将要讨论的非阻塞connect技术
* 客户端程序要同时处理用户输入和网络连接，本章要讨论的聊天室程序
* TCP服务器要同时处理监听socket和连接socket
* 服务器要同时处理TCP请求和UDP请求
* 服务器要同时监听多个端口，或者处理多中服务

需要指出的是 IO复用虽然能同时监听多个文件描述符，但是它本身就是阻塞的。并且当多个文件描述符同时就绪时，如果不采取额外的措施，程序就只能按顺序依次处理其中的每个文件描述符，这使得服务器程序看起来像是串行工作的，如果要实现并发，只能使用多进程或多线程等编程手段

 Linux下实现IO复用的系统调用主要是select、poll和epoll

 ## select系统调用

 select系统调用的用途是：在一段时间内，监听用户感兴趣的文件描述符上的可读、可写和异常等时间。
 
 接下来先介绍select系统调用的API，然后再讨论select判断文件描述符就绪的条件，最后给出它在处理带外数据中的应用

 ### select API

 select系统调用的原型如下

 ```
 #include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
 ```
 
 * nfds参数指定被监听的文件描述符的总数，通常被设置为select监听的所有文件描述符中的最大值加1，因为文件描述符是从0开始计数的

 * readfds、writefds和exceptfds参数分别指向可读、可写和异常等事件对应的文件描述符集合。应用程序调用select函数时，通过这3个参数传入自己感兴趣的文件描述符。select调用返回时，内核将修改它们来通知应用程序哪些文件描述符已经就绪

    `fd_set`结构体仅包含一个整型数组，该数组的每个元素的每一位标记一个文件描述符，所以`fd_set`能容纳的文件描述符数量由FD_SETSIZE制定，这就限制了select能同时处理的文件描述符的总量

    ```
    #include <sys/select.h>
    FD_ZERO(fd_set *fdset); // 清楚fdset的所有位
    FD_SET(int fd, fd_set *fdset); // 设置fdset的位fd
    FD_CLR(int fd, fd_set *fdset); // 清楚fdset的位fd
    int FD_ISSET(int fd, fd_set *fdset); // 测试fdset的位fd是否被设置
    ```

* timeout参数用来设置select函数的超时事件，它是一个timeval结构类型的指针，采用指针参数是因为内核将修改它以告诉应用程序select等待了多久，不能完全信任，因为有时候调用失败时timeout值是不确定的。

    ```
    struct timeval
    {
        long tv_sec; // 秒数
        long tv_usec; // 微秒数
    }
    ```

    如果都传递给0，则立即返回，如果是NULL，则一直阻塞，直到某个文件描述符就绪


select成功时返回就绪文件描述符的总数。如果在超时时间内没有任何描述符就绪，则为0，失败时返回-1并设置errno

### 文件描述符就绪条件

下列情况下socket可读：

* socket内核接收缓存区中的字节数大于或等于其低水位标记SO_RCVLOWAT,此时我们可以无阻塞的读该socket，并且读操作返回的字节数大于0

* socjet通信的对方关闭连接，此时对该socket的读操作返回0

* 监听socket上有新的连接请求

* socket上有未处理得错误，此时我们可以使用getsockopt来读取和清除该错误

下列情况下socket可写

* socket内核发送缓冲区的可用字节数大于等于其低水位标记SO_SNDLOWAT,此时我们可以无阻塞地写该socket，并且写操作返回的字节数大于0

* socket的写操作被关闭，对写操作被关闭的socket执行写操作将触发一个SIGPIPE信号

* socket使用非阻塞connect连接成功或失败之后

* socket上犹未处理的错误，我们可以使用getsockopt来读取和清除该错误

出现异常的情况只有：socket上接收到带外数据

