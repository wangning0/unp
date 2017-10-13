# 高级I／O函数

讨论网络编程相关的几个，这些函数大致分为三类：
* 用于创建文件描述符的函数，包括pipe、dup/dup2函数
* 用于读写数据的函数，包括readv/writev、sendfile、mmap/munmap、splice和tee函数
* 用于控制I/O行为和属性的函数，包括fcntl函数

## pipe函数

pipe函数可用于创建一个管道，已实现进程间通信。pipe函数的定义如下：

```
#include <unistd.h>
int pipe(int fd[2]);
```
pipe函数的参数是一个包含两个int型整数的数组指针。该函数成功时返回0，并将一对打开的文件描述符值填入其参数指向的数组，如果失败，则返回-1并设置errno

通过pipe函数创建的这两个文件描述符fd[0]和fd[1]分别构成管道的两端，往fd[1]写入的数据可以从fd[0]读出，并且，fd[0]只能用于从管道读出数据，fd[1]则只能用于往管道写入数据，而不能反过来使用。如果要实现双向的数据传输，就应该使用两个管道。

默认情况下，这一块文件描述符都是阻塞的，此时如果我们用read系统调用来读取一个空的管道，则read被阻塞，直到管道内有数据可读；如果我们用write系统调用来往一个满的管道中写入数据，则write也被阻塞，直到管道又足够多的空闲空间可用。

但如果应用程序将fd[0]和fd[1]都设置为非阻塞的，则read和write有不同的行为。

如果管道的写端文件描述符fd[1]的引用计数减少为0，即没有任何进程需要往管道中写入数据，则针对该管道的读端文件描述符fd[0]的read操作将返回0，即读取到了文件结束标记(EOF)

如果管道的读端文件描述符fd[0]的引用计数减少至0，即没有任何进程需要从管道读取数据，则针对该管道的写端文件描述符fd[1]的write操作将失败，并引发SIGPIPE信号。

管道内部传输的数据是字节流，这和TCP字节流的概念相同。但两者又有细微的区别。

应用程序能往一个TCP连接中写入多少字节的数据，取决于对方的接收通告窗口的大小和本端的拥塞窗口的大小。而管道本身拥有一个容量限制，规定该管道最多能被写入多少字节的数据，现在管道容量的大小默认是65536字节，可以使用fcntl函数来修改管道容量

socketpair函数，能够方便地创建双向管道
```
#include <sys/types.h>
#include <sys/socket.h>
int socketpair(int domain, int type, int protocol, int fd[2]);
```

前三个参数的含义和socket系统调用的三个参数相同，但是domain只能使用AF_UNIX，最后一个参数和pipe系统调用的参数一样，只不过socketpair创建的这对文件描述符是可读可写的

## dup函数和dup2函数

有时候我们希望把标准输入重定向到一个文件，或者把标准输出重定向到一个网络连接，这可以通过下面的用于复制文件描述符的dup和dup2函数来实现

```
#include <unistd.h>
int dup(int file_descriptor);
int dup2(int file_descriptor_one, int file_descriptor_two);
```

dup函数创建一个新的文件描述符，该新文件描述符和原有文件描述符file_scriptor指向相同的文件、管道或者网络连接。并且dup返回的文件描述符总是取系统当前可用的最小整数值。dup2和dup类似，不过它将返回一个不小于`file_descriptor_two`的整数值。dup和dup2系统调用失败时返回-1并设置errno

## readv函数和writev函数

readv函数将数据从文件描述符读到分散的内存块，即分散读；
writev函数则将多块分散的内存数据一并写入文件描述符中，即集中写。

```
#include <sys/uio.h>
ssize_t readv(int fd, const struct iovec* vector, int count);
ssize_t writev(int fd, const struct iovec* vector, int count);
```

```
struct iovec {
    void* iov_base; // pointer to data
    size_t iov_len; // length of data
}
```
fd参数是被操作的目标文件描述符，vector参数的类型是iovec结构数组，该结构体描述的是一块内存区。count参数是vector数组的长度，即有多少块内存数据需要从fd读出或写到fd。readv和writev在成功时返回读出／写入fd的字节数，失败返回-1并设置errno

## sendfile函数

sendfile函数在两个文件描述符之间直接传递数据，从而避免了内核缓冲区和用户缓冲区指尖的数据拷贝，效率很高，这就称为零拷贝

```
#include <sys/sendfile.h>
ssize_t sendfile(int put_fd, int in_fd, off_t* offert, size_t count);
```

in_fd参数是待读出内容的文件描述符，`out_fd`参数是待写入内容的文件描述符。offset参数指定从读入文件流的哪个位置开始读，如果为空，则使用文件流默认的起始位置。

count参数指定在文件描述符in_fd和out_fd之间传输的字节数。sendfile成功时返回传输的字节数，失败则返回-1并设置errno

in_fd必须是一个支持类似mmap函数的文件描述符，即它必须指向真实的文件，不能是socker和管道，而out_fd则必须是一个socket

