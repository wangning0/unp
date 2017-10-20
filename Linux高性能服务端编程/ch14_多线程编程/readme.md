# 多线程编程

自内核2.6开始，Linux才真正提供内核级的线程支持，并有两个组织致力于编写新的线程库：NGPT和NPTL，不过现在只有NPTL，所以新的线程哭库就称为NPTL.NPTL比LInuxThreads效率高，更符合POSIX规范，所以它已经成为glibc的一部分。

本书所有线程相关的使用的线程库都是NPTL。

本章包括：

* 创建线程和结束线程
* 读取和设置线程属性
* POSIX线程同步方式： POSIX信号量、互斥锁和条件变量


## Linux线程概述

### 线程模型

线程是程序中完成一个独立任务的完整执行序列，即一个可调度的实体。

根据运行环境和调度者的身份，线程可分为内核线程和用户线程。

* 内核线程，在有的系统上也称为LWP，轻量级进程，运行在内核空间，由内核来调度。

* 用户线程运行在用户空间，由线程库来调度。

当进程的一个内核线程获得CPU的使用权时，它就加载并运行一个用户线程。可见，内核线程相当于用户线程运行的“容器”，一个进程可以拥有M个内核进程和N个用户线程，并且M<=N,。并且在一个系统的所有进程中，M和N的比值都是固定的，按照M:N的取值，线程的实现方式可分为三种模式，完全在用户空间实现、完全由内核调度和双层调度

线程的优先级只对同一个进程的线程有效，比较不同进程中的线程的优先级没有意义。

* 完全在用户空间实现的线程无须内核的支持，内核甚至都不知道线程的存在，线程库负责管理所有执行线程。即N个用户线程对应一个内核线程，而该内核线程实际上就是进程本身。完全在用户空间实现的线程的优点是：创建和调度线程都无需内核的干预，因此速度很快，并且不占用额外的内核资源，所以即使一个进程创建了很多线程，也不会对系统性能造成明显的影响，缺点是：对于多处理器系统，一个进程的多个线程无法运行在不同的CPU上，因为内核是按照其最小调度单位来分配CPU的。

* 完全由内核调度的魔术将创建、调度线程的人物都交给了内核，运行在用户空间的线程库无须执行管理任何，这和完全在用户空间实现的线程恰恰相反，两者的优缺点也正好互换。

* 双层调度模式是前两种模式的混合体：内核调度M个内核线程，线程哭调度N个用户现场，结合了前两种方式的优点，不但不会消耗过多的内核资源，而且线程切换速度也快，同时可以充分利用多处理器的优势

### Linux线程库

NPTL的主要优势在于 

* 内核线程不再是一个进程，因此避免了很多用进程模拟内核线程导致的语义问题
* 摒弃了管理线程，终止线程、回收线程堆栈等工作都可以由内核完成
* 由于不存在管理线程，所以一个进程的线程可以运行在不同的CPU上，从而充分利用了多处理器系统的优势
* 线程的同步由内核来完成，隶属于不同进程的线程之间可以共享互斥锁，可实现跨进程的线程同步


## 创建线程和结束线程

这些API，都定义在pthread.h头文件中

* pthread_create

    创建一个线程的函数

    ```
    #include <pthread.h>
    int pthread_create(pthread_t *thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
    ```

    thread参数试线程的标识符，后续`pthread_*`函数通过它来饮用线程，类型定义

    ```
    #include <bits/pthreadtypes.h>
    typedef unsigned long int pthread_t;
    ```
    可见`pthread_t`是一个整型类型，实际上Linux所有的资源标识符都是一个整数。

    attr参数用于设置新线程的类型，传递NULL表示默认线程属性，线程有许多属性

    `start_routine`和arg参数分别指定新线程将运行的函数及其参数

    该系统调用成功时返回0，失败时返回错误码，所用用户能创建的线程总数不得超过`/proc/sys/kernel/threads-max`内核参数所定义的值

* `pthread_exit`

    线程一旦被创建好，内核就可以调度内核线程来执行`start_routine`函数指针所指向的函数了，线程函数在结束时最好调用如下函数，确保安全地、干净地退出

    ```
    #include <pthread.h>
    void pthread_exit(void *retval)
    ```
    该函数通过retval参数向线程的回收者传递其退出信息，永远不会失败

* `pthread_join`

    一个线程中所有的线程都可以调用`pthread_join`函数来回收其他线程，如果目标线程可以回收，即等待其他线程结束，这类似回收进程的wait和waitpid系统调用

    ```
    #include <pthread.h>
    int pthread_join(pthread_t thread, void** retval);
    ```

    thread参数是目标线程标识符，retval参数则是目标线程返回的退出信息，该函数一致阻塞，直到被回收的线程结束为止。该函数成功时返回0，失败返回错误码

    错误码：

    * EDEADLK，死锁，互相调用`pthread_join`，或者对自身调用`pthread_join`

    * EINVAL 目标线程是不可回收的，或者已经有其他线程在回收该目标线程

    * ESRCH 目标线程不存在

* `pthread_cancel` 

    我们希望异常终止一个线程，即取消线程

    ```
    #include <pthread.h>
    int pthread_cancel(pthread_t thread);
    ```

    成功为0，失败返回错误码，不过接收到取消请求的目标线程可以决定是否允许被取消以及如何取消

    ```
    #include <pthread.h>
    int pthread_setcancelstate(int state, int *oldstate);
    int pthread_setcanceltype(int type, int *oldtype);
    ```

    第一个参数设置线程的取消状态和取消类型，第二个参数分别记录线程原来的取消状态和取消类型

    state的可选：

    * `PTHREAD_CANCEL_ENABLE` 允许

    * `PTHREAD_CANCEL_DISABLE` 禁止

    type参数

    * `PTHREAD_CANCEL_ASYNCHRONOUS`线程随时都可以被取消

    * `PTHREAD_CANCEL_DEFERRED`允许目标线程推迟行动

    成功返回0，失败返回错误码

## 线程属性

`pthread_attr_t`结构体定义了一套完整的线程属性

```
#include  <bits/pthreadtypes.h>
#define __SIZEOF_PTHREAD_ATTR_T 36
typedef union
{
    char __size[__SIZEOF_PTHREAD_ATTR_T];
    long int __align;
} pthread_attr_t;
```

一系列函数来操作`pthread_attr_t`类型的变量，方便我们获取和设置线程属性

```
#include <pthread.h>
// 初始化线程属性对象
int pthread_attr_init (pthread_attr_t* attr);
//销毁属性对象
int pthread_attr_destroy(pthread_attr_t* attr);
// 下面这些函数用于获取和设置线程属性对象中的某个属性
int pthread_attr_getdetachstate(const pthread_attr_t* attr, int *detachstate);
int pthread_attr_setdetachstate(const pthread_attr_t* attr, int detachstate);

int pthread_attr_getstackaddr(const pthread_attr_t*attr, void ** stackaddr);
int pthread_attr_setstackaddr(const pthread_attr_t*attr, void * stackaddr);

int pthread_attr_getstacksize(const pthread_attr_t*attr, size_t *stacksize);
int pthread_attr_setstacksize(const pthread_attr_t*attr, size_t stacksize);

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);  
int pthread_attr_getschedpolicy(pthread_attr_t *attr, int *policy);  

int pthread_attr_setschedparam(pthread_attr_t *attr,  
                               const struct sched_param *param);  
int pthread_attr_getschedparam(pthread_attr_t *attr,  
                               struct sched_param *param);  


int pthread_attr_setinheritsched(pthread_attr_t *attr,  
                                 int inheritsched);  
int pthread_attr_getinheritsched(pthread_attr_t *attr,  
                                 int *inheritsched);  

int pthread_attr_setscope(pthread_attr_t *attr, int scope);  
int pthread_attr_getscope(pthread_attr_t *attr, int *scope); 

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);  
int pthread_attr_getguardsize(pthread_attr_t *attr, size_t *guardsize); 

```

* detachstate，线程的脱离状态。它有`PTHREAD_CREATE_JOINABLE`和`PTHREAD_CREATE_DETACH`两个可选值。前者指定线程是可以被回收的，后者使调用线程与进程中其他线程的同步，脱离了与其他线程同步的线程称为脱离现场，脱离线程在退出时候会自行释放其占用的系统资源，默认值是可回收的

* stackaddr和stacksize，线程堆栈的起始地址和大小，一般来说，我们不需要自己来管理线程堆栈，因为Linux默认为每个线程分配了足够的堆栈空间

* guardsize 保护区域大小，如果guardsize大于0，则系统创建线程的时间会在其堆栈的尾部额外分配guardsize字节的空间，作为保护堆栈不被错误地覆盖的区域。如果等于0，则不为新创建的线程设置堆栈保护区。如果手动设置了线程的堆栈，则guardsize属性被忽略

* schedparam，线程调度参数。类型是`sched_param`结构体，只有一个`sched_priority`表示线程的运行优先级

* schedpolicy 线程调度策略，先入先出策略 (`SCHED_FIFO`)、循环策略 (`SCHED_RR`) 和自定义策略 (`SCHED_OTHER`)。`SCHED_FIFO` 是基于队列的调度程序，对于每个优先级都会使用不同的队列。`SCHED_RR` 与 FIFO 相似，不同的是前者的每个线程都有一个执行时间配额。`SCHED_FIFO` 和 `SCHED_RR` 是对 POSIX Realtime 的扩展。`SCHED_OTHER` 是缺省的调度策略。
新线程默认使用 `CHED_OTHER` 调度策略。线程一旦开始运行，直到被抢占或者直到线程阻塞或停止为止

* inheritsched `THREAD_EXPLICIT_SCHED`
 `PTHREAD_INHERIT_SCHED` 前者表示使用结构pthread_attr_t指定的调度算法，后者表示继承父线程使用的调度算法，默认为前者

 * scope, `PTHREAD_SCOPE_SYSTEM` `PTHREAD_SCOPE_PROCESS` 前者表示在整个系统内竞争CPU资源，后者表示在同一进程内竞争CPU资源，默认为前者

 ## POSIX 信号量

 `pthread_join`可以看作一种简单的线程同步方式，但是无法高效实现复杂的同步需求，比如控制对共享资源的独占式访问，或者是在某个条件满足之后唤醒一个线程
 
 接下来将三种专门用于线程同步的机制 POSIX信号量，互斥量和条件变量

 ```
 #include <semaphore.h>
 int sem_init(sem_t* sem, int pshaed, unsigned int value);
 int sem_destroy(sem_t* sem);
 int sem_wait(sem_t* sem);
 int sem_trywaite(sem_t* sem);
 int sem_post(sem_t* sem);
 ```

 第一个参数sem只想被操作的信号量

 `sem_init` 初始化一个未命名的信号量，， pshaed表示信号量的类型，0表示局部信号量，value表示信号量的初始值

 `sem_destroy` 销毁信号量

 `sem_wait` `sem_trywaite`表示减1

 `sem_post`表示加1

 成功时返回0，失败返回-1，并设置errno

 ## 互斥锁

 互斥锁/互斥量 可以用于保护关键代码段／临界区，以确保其独占式的访问，有点像二进制信号量。进入加锁，P操作，离开，解锁，V操作。

 ### 互斥锁基础API

 ```
 #include <pthread.h>
 int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr); // 初始化
 int pthread_mutex_destory(pthread_mutex_t* mutex); // 销毁
 int pthread_mutex_lock(pthread_mutex_t* mutex); // 加锁
 int pthread_mutex_trylock(pthread_mutex_t* mutex); // 非阻塞版加锁，已经锁了EBUSY返回，否则就加锁 
 int pthread_mutex_unlock(pthread_mutex_t* mutex); // 释放锁
 ```

函数的第一个参数mutex指向要操作的目标互斥锁，类型是`pthread_mutex_t`结构体

成功返回0，失败返回错误码

### 互斥锁属性

`pthread_mutexattr_t`结构体定义了一套完整的互斥锁属性，线程库提供了一系列函数来操作`pthread_mutexattr_t`类型的变量，方便我们获取和设置互斥锁属性

```
#include <pthread.h>
// 初始化互斥属性对象
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destory(pthread_mutexattr_t* attr);
int pthread_mutexattr_getpshared(const pthread_mutexattr_t* attr, int* pshared);
int pthread_mutexattr_setpshared(pthread_mutexattr_t* attr, int pshared);
int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* type);
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);
```

只讨论互斥锁的两种常用属性： pshared和type。互斥锁属性pshared指定是否允许跨进程共享互斥锁

* `PTHREAD_PROCESS_SHARED` 互斥锁可以被跨进程共享
* `PTHREAD_PROCESS_PRIVATE` 互斥锁只能被和锁的初始化线程隶属于同一个进程的线程共享

互斥锁属性type制定互斥锁的类型

* `PTHREAD_MUTEX_NORMAL` 普通锁，互斥锁的默认类型，当一个线程对一个普通所加锁以后，其余请求该锁的线程将形成一个等待队列，并在该锁解锁后按优先级获得它。一个线程如果对一个已经加锁的普通锁再次加锁，将引发死锁，对一个已经被其他线程加锁的普通锁加锁，或者对一个已经解锁的普通锁再次解锁，将导致不可预期的后果

* `PTHREAD_MUTEX_ERRORCHECK` 检错锁。一个线程如果对一个已经加锁的检错锁再次加锁，则加锁操作返回EDEADLK


* `PTHREAD+MUTEX_RECURSIVE` 嵌套锁。

* `PTHREAD_MUTEX_DEFAULT`默认锁。一个线程如果对一个已经加锁的默认锁再次加锁。或者对一个已经被其他线程加锁的默认锁解锁，或者对一个已经解锁的默认锁再次解锁，将导致不可预期的后果


## 条件变量

如果说互斥锁事用于同步线程对共享数据的访问的话，那么条件变量是用于在线程之间同步共享数据的值。条件变量提供了一种线程间的通知机制，当某个共享数据达到某个值的时候，唤醒等待这个共享数据的线程

```
#include <pthread.h>
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr);
int pthread_cond_destory(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
```

第一个参数cond指向要操作的目标条件变量，条件变量的类型是`pthread_cond_t`结构体

`pthread_cond_init`函数用于初始化条件变量，`cond_attr`参数指定条件变量的属性，如果将它设置成NULL，则表示使用默认属性

`pthread_cond_destory`销毁条件变量

`pthread_cond_broadcast`函数以广播的方式唤醒所有等待目标条件变量的线程

`pthread_cond_signal`函数用来唤醒一个等待目标条件变量的线程，取决于线程的优先级和调度策略

`pthread_cond_wait`函数用于等待目标条件变量，mutex参数是用于保护条件变量的互斥锁，确保该函数的原子性


## 线程和进程

如果一个多线程程序的某个线程调用了fork函数，那么新创建的紫禁城是否将自动创建和父进程相同数量的线程呢？答案是否定的，

子进程只有一个执行线程，就是调用fork的那个线程的完整复制。并且子进程将自动继承父进程中互斥锁的状态。

会导致一个问题：子进程可能不清楚从父进程继承而来的互斥锁的具体状态。这个互斥锁可能被加锁了，但并不是由调用fork函数的那个线程锁住的，而是由其他线程锁住的，如果是这样，则子进程在此对该互斥锁加锁操作会导致死锁

