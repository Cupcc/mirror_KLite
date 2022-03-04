# KLite 参考手册
	更新日期: 2022.02.22

## 一、简介

**KLite** 是由个人开发者编写的嵌入式操作系统内核，创建于2015年5月6日，并以MIT协议开放源代码。  
KLite的定位是一款入门级的嵌入式实时操作系统内核，以简洁易用为设计目标，旨在降低学习嵌入式RTOS的入门难度。  
代码干净工整，架构清晰，模块化设计简单明了，函数简单易用，不使用条件编译，移植简单，无需复杂的配置和裁减，可能是目前最简洁易用的RTOS。
________________________________________________________________________________
**功能特性：**
- 资源占用极小（ROM：2KB，RAM：0.5KB）
- 优先级抢占
- 时间片抢占（相同优先级）
- 支持创建相同优先级的线程
- 丰富的线程通信机制  
- 动态内存管理
- 多编译器支持（GCC,IAR,KEIL）

**作者简介：** 蒋晓岗，男，现居成都，2013年毕业于成都信息工程大学计算机科学与技术专业，从事嵌入式软件开发工作近十年。

## 二、开始使用

KLite目前支持ARM7、ARM9、Cortex-M0、Cortex-M3、Cortex-M4、Cortex-M7内核，若你的MCU是基于以上平台如：全志F1C100S、STM32、NRF51/52等，请参考相关例程。  
对于Cortex-M架构的MCU，实际上只需要修改cmsis.c里面的#include，比如#include "stm32f10x.h"
> 由于KLite源码不使用条件编译，因此可以直接预编译kernel源码为kernel.lib，并保留kernel.h，可以有效减少重复编译的时间。

main.c的推荐写法如下:
```
//只需要包含这一个头文件
#include "kernel.h"

//用于初始化应用程序的线程
void init(void *arg)
{
	//在这里完成外设和驱动初始化
	//并创建更多线程实现不同的功能
	//thread_create(...)
}

//空闲线程，只需调用kernel_idle即可
void idle(void *arg)
{
	kernel_idle();
}

//C语言程序入口
void main(void)
{
	static uint8_t heap[HEAP_SIZE]; /* 定义堆内存 */
	kernel_init(heap, sizeof(heap)); /* 系统初始化 */
	thread_create(idle, 0, 0); /* 创建idle线程 */
	thread_create(init, 0, 0); /* 创建init线程 */
	kernel_start(); /* 启动系统 */
}
```

## 三、核心功能

核心功能的源码在sources/kernel/目录，是KLite最核心的部分使用这些功能，只需要包含#include "kernel.h"就可以。

### 3.1 内核管理

+ void kernel_init(void *heap_addr, uint32_t heap_size);  
	参数：`heap_addr` 动态分配起始地址  
	参数：`heap_size` 动态分配内存大小  
	返回：无  
	描述：用于内核初始化在调用内核初始化时需保证中断处于关闭状态，
	此函数只能执行一次，在初始化内核之前不可调用内核其它函数。

________________________________________________________________________________
+ void kernel_start(void);  
	参数：无  
	返回：无  
	描述：用于启动内核，此函正常情况下不会返回，在调用之前至少要创建一个线程

________________________________________________________________________________
+ uint32_t kernel_version(void);  
	参数：无  
	返回：KLite版本号，BIT[31:24]主版本号，BIT[23:16]次版本号，BIT[15:0]修订号  
	描述：此函数可以获取KLite的版本号
	> 命名规则：  
	> 主版本：架构调整，比较大的修改，与旧版本可能会不兼容  
	> 次版本：功能调整，主要涉及新增或删除功能  
	> 修订号：细节优化或BUG修复  

________________________________________________________________________________
+ void kernel_idle(void);  
	参数：无  
	返回：无  
	描述：处理内核空闲事务，回收线程资源此函数不会返回。必须单独创建一个线程来调用。
	> 由用户创建空闲线程是为了实现灵活配置空闲线程的stack大小，不使用宏定义来进行stack的配置。
	如果使用宏定义来配置stack大小，那么代码编译成lib之后就无法修改了，失去了灵活性。

________________________________________________________________________________
+ void kernel_tick(uint32_t time);  
	参数：滴答周期(毫秒)  
	返回：无  
	描述：此函数不是用户API，而是由CPU的滴答时钟中断程序调用，为系统提供时钟源。  
	滴答定时器的周期决定了系统计时功能的细粒度，主频较低的处理器推荐使用10ms周期，主频较高则使用1ms周期。  
	> 我们在这里把滴答时间转为毫秒单位，应用层就不必使用宏定义来进行单位转换，起到简化调用的目的。  
	如果硬件定时器不能产生1ms的时钟，比如RTC=32768Hz，只能产生1024Hz的中断源，周期为0.97ms，这就很尴尬！  
	方案一：软件修正误差，在1024个周期内，均匀地跳过24次中断。  
	方案二：设置中断周期为125ms，这是在32768Hz时钟下能得到的最小整数时间。  
	方案三：放弃毫秒为时间单位，老老实实用滴答数做为时间单位。

________________________________________________________________________________
+ uint32_t kernel_tick_count(void);  
	参数：无  
	返回：系统运行时间(毫秒)  
	描述：此函数可以获取内核从启动到现在所运行的总时间  

________________________________________________________________________________
+ uint32_t kernel_tick_idle(void);  
	参数：无  
	返回：系统空闲时间(毫秒)  
	描述：获取系统从启动到现在空闲线程占用CPU的总时间，可使用此函数和`kernel_tick_count()`一起计算CPU占用率

### 3.2 内存管理

________________________________________________________________________________
+ void *heap_alloc(uint32_t size);  
	参数：`size` 要申请的内存大小  
	返回：申请成功返回内存指针，申请失败返回`NULL`  
	描述：从堆中申请一段连续的内存，功能和标准库的`malloc()`一样  
	> 严格来说调用此函数应该检查返回值是否为`NULL`，
	但每次`heap_alloc`都要写检查返回值的代码可能有点繁琐或者容易有遗漏的地方，
	所以此函数在返回`NULL`之前会调用一个HOOK函数，原型`void heap_fault(void);`
	对于嵌入式系统来说申请内存失败是严重错误，所以我们可以偷懒只在`heap_fault()`函数中统一处理错误。

________________________________________________________________________________
+ void heap_free(void *mem);  
	参数：`mem` 要释放的内存指针  
	返回：无  
	描述：释放由`heap_malloc()`申请的内存，功能和标准库的`free()`一样  

________________________________________________________________________________
+ void heap_usage(uint32_t *used, uint32_t *free);  
	参数：  
	`used` 输出已使用内存数量(字节)，此参数可以为`NULL`  
	`free` 输出空闲内存数量(字节)，此参数可以为`NULL`  
	返回：无  
	描述：获取内存用量信息，可使用此函数关注系统内存消耗，以适当调整内存分配  

### 3.3 线程管理

+ thread_t thread_create(void(*entry)(void*), void *arg, uint32_t stack_size);  
	参数：`entry` 线程入口函数  
	参数：`arg` 线程入口函数的参数  
	参数：`stack_size` 线程的栈大小（字节），为0则使用系统默认值(1024字节)  
	返回：成功返回线程句柄，失败返回`NULL`  
	描述：创建新线程，并加入就绪队列 
	> 系统自动为新线程分配内存和栈空间，如果栈设置太小运行过程中可能会产生栈溢出  

________________________________________________________________________________
+ void thread_delete(thread_t thread);  
	参数：`thread`  被删除的线程标识符  
	返回：无  
	描述：删除线程，并释放内存该函数不能用来结束当前线程如果想要结束当前线程请使用`thread_exit()`或直接使用return退出主循环  
	> 不推荐直接删除线程，可能会造成系统不稳定，因为被删除线程可能进入了临界区未释放，需考虑清楚再使用  

________________________________________________________________________________
+ void thread_set_priority(thread_t thread, uint32_t prio);  
	参数：`thread` 线程标识符  
	参数：`prio` 新的优先级  
	> `THREAD_PRIORITY_HIGHEST` 最高优先级  
	> `THREAD_PRIORITY_HIGHER` 更高优先级  
	> `THREAD_PRIORITY_HIGH` 高优先级  
	> `THREAD_PRIORITY_NORMAL` 默认优先级  
	> `THREAD_PRIORITY_LOW` 低优先级  
	> `THREAD_PRIORITY_LOWER` 更低优先级  
	> `THREAD_PRIORITY_LOWEST` 最低优先级  
	> `THREAD_PRIORITY_IDLE` 空闲优先级  
	
	返回：无  
	描述：重新设置线程优先级，立即生效。  

________________________________________________________________________________
+ uint32_t thread_get_priority(thread_t thread);  
	参数：`thread` 线程标识符  
	返回：目标线程的优先级  
	描述：获取指定线程的优先级  

________________________________________________________________________________
+ thread_t thread_self(void);  
	参数：  无  
	返回： 调用线程的标识符  
	描述： 用于获取当前线程标识符  

________________________________________________________________________________
+ void thread_sleep(uint32_t time);  
	参数：`time` 休眠时间（毫秒）  
	返回：无  
	描述：将当前线程休眠一段时间，释放CPU控制权  

________________________________________________________________________________
+ void thread_yield(void);  
	参数：无  
	返回：无  
	描述：使当前线程立即释放CPU控制权，并进入就绪队列  

________________________________________________________________________________
+ void thread_exit(void);  
	参数：无  
	返回：无  
	描述：退出当前线程，此函数不会立即释放线程占用的内存，需等待系统空闲时释放  

________________________________________________________________________________
+ void thread_suspend(void);  
	参数：无  
	返回：无  
	描述：挂起当前线程  
	> 此函数故意设计为没有参数，不能挂起指定线程，原因与不推荐使用`thread_delete`一样  

________________________________________________________________________________
+ void thread_resume(thread_t thread);  
	参数：`thread` 线程标识符  
	返回：无  
	描述：恢复被挂起的线程  
	> 如果指定线程没有被挂起，可能会导致意外行为  

________________________________________________________________________________
+ uint32_t thread_time(thread_t thread)  
	参数：`thread` 线程标识符  
	返回：指定线程运行时间（毫秒）  
	描述：获取线程自创建以来所占用CPU的时间在休眠期间的时间不计算在内.  
	> 可以使用此函数来监控某个线程的CPU占用率  

### 3.4 互斥锁
________________________________________________________________________________
+ mutex_t mutex_create(void)  
	参数：无  
	返回：成功返回互斥锁标识符，失败返回`NULL`  
	描述：创建一个互斥锁对象，支持递归锁
________________________________________________________________________________
+ void mutex_delete(mutex_t mutex)  
	参数：`mutex` 被删除的互斥锁标识符  
	返回：无  
	描述：删除一个互斥锁，并释放内存
	> 注意：在删除互斥锁的时候不会释放等待这个锁的线程，因此在删除之前请确认没有线程在使用它
________________________________________________________________________________
+ void mutex_lock(mutex_t mutex)  
	参数：`mutex` 互斥锁标识符  
	返回：无  
	描述：将`mutex`指定的互斥锁标记为锁定状态，如果`mutex`已被其它线程锁定，则调用线程将会被阻塞，直到另一个线程释放这个互斥锁  
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009716  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_lock.html  
________________________________________________________________________________
+	bool mutex_try_lock(mutex_t mutex)  
	参数：`mutex` 互斥锁标识符  
	返回：如果锁定成功则返回`true`，失败则返回`false`  
	描述：此函数是`mutex_lock`的非阻塞版本  
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009721  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_trylock.html  
________________________________________________________________________________
+ void mutex_unlock(mutex_t mutex)  
	参数：`mutex` 互斥锁标识符  
	返回：无  
	描述：释放`mutex`标识的互斥锁，如果有其它线程正在等待这个锁，则会唤醒优先级最高的那个线程  
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009722  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_unlock.html  

### 3.5 信号量
________________________________________________________________________________
+ sem_t sem_create(uint32_t value)  
	参数：`value` 信号量初始值  
	返回：成功返回信号量标识符，失败返回`NULL`  
	描述：创建信号量对象  
	> 参考：  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_init.html
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsemaphorea
________________________________________________________________________________
+ void sem_delete(sem_t sem)  
	参数：`sem` 信号量标识符  
	返回：无  
	描述：删除对象，并释放内存在没有线程使用它时才能删除，否则可能产生未知异常  
________________________________________________________________________________
+ void sem_post(sem_t sem)
	参数：`sem` 信号量标识符  
	返回：无  
	描述：信号量计数值加1，如果有线程在等待信号量，此函数会唤醒优先级最高的线程  
	> 参考：  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_post.html  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasesemaphore  
________________________________________________________________________________
+ void sem_wait(sem_t sem)  
	参数：`sem` 信号量标识符  
	返回：无  
	描述：等待信号量，信号量计数值减1，如果当前信号量计数值为0，则线程阻塞，直到计数值大于0  
	> 参考：  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_wait.html  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject  
________________________________________________________________________________
+ uint32_t sem_timed_wait(sem_t sem, uint32_t timeout)  
	参数：`sem` 信号量标识符  
	参数：`timeout` 超时时间（毫秒）  
	返回：剩余等待时间，如果返回0则说明等待超时  
	描述：定时等待信号量，并将信号量计数值减1，如果当前信号量计数值为0，则线程阻塞，直到计数值大于0，或者阻塞时间超过`timeout`指定的时间
	> 参考：  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_timedwait.html  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject  
________________________________________________________________________________
+ uint32_t sem_value(sem_t sem)  
	参数：`sem` 信号量标识符  
	返回：信号量计数值  
	描述：返回信号量计数值  
	> 参考：  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_getvalue.html  
	
### 3.6 条件变量
________________________________________________________________________________
+ cond_t cond_create(void);  
	参数：无  
	返回：成功返回条件变量标识符，失败返回`NULL`  
	描述：创建条件变量对象  
________________________________________________________________________________
+ void cond_delete(cond_t cond);  
	参数：`cond` 条件变量标识符  
	返回：无  
	描述：删除条件变量  
________________________________________________________________________________
+ void cond_signal(cond_t cond);  
	参数：`cond` 条件变量标识符  
	返回：无  
	描述：唤醒一个被条件变量阻塞的线程，如果没有线程被阻塞则此函数什么也不做  
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009711  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_signal.html  
________________________________________________________________________________
+ void cond_broadcast(cond_t cond);  
	参数：`cond` 条件变量标识符  
	返回：无  
	描述：唤醒全部被条件变量阻塞的线程，如果没有线程被阻塞则此函数什么也不做   
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009708  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_broadcast.html  
________________________________________________________________________________
+ void cond_wait(cond_t cond, mutex_t mutex);  
	参数：`cond` 条件变量标识符  
	参数：`mutex` 互斥锁标识符  
	返回：无  
	描述：阻塞线程，并等待被条件变量唤醒  
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009713  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_wait.html  
________________________________________________________________________________
+ uint32_t cond_timed_wait(cond_t cond, mutex_t mutex, uint32_t timeout);  
	参数：`cond` 条件变量标识符  
	参数：`mutex` 互斥锁标识符  
	参数：`timeout` 超时时间（毫秒）  
	返回：剩余等待时间，如果返回0则说明等待超时  
	功能：定时阻塞线程，并等待被条件变量唤醒  
	> 参考：  
	> C11：https://cloud.tencent.com/developer/section/1009712  
	> POSIX：https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_timedwait.html  

### 3.7 事件
________________________________________________________________________________
+ event_t event_create(bool auto_reset);  
	参数：`auto_reset` 是否自动复位事件  
	返回：创建成功返回事件标识符，失败返回`NULL`  
	描述：创建一个事件对象，当`auto_reset`为`true`时事件会在传递成功后自动复位  
	> 参考：  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventa  
________________________________________________________________________________
+ void event_delete(event_t event);  
	参数：`event` 事件标识符  
	返回：无  
	描述：删除事件对象，并释放内存，在没有线程使用它时才能删除，否则可能产生未知异常  
________________________________________________________________________________
+ void event_set(event_t event);  
	参数：`event` 事件标识符  
	返回：无  
	描述：标记事件为置位状态，并唤醒等待队列中的线程，如果`auto_reset`为`true`，那么只唤醒第1个线程，并且将事件复位，
	如果`auto_reset`为`false`，那么会唤醒所有线程，事件保持置位状态  
	> 参考：  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent  
________________________________________________________________________________
+ void event_reset(event_t event);  
	参数：`event` 事件标识符  
	返回：无  
	描述：标记事件为复位状态，此函数不会唤醒任何线程  
	> 参考：  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent  
________________________________________________________________________________
+ void event_wait(event_t event);  
	参数：`event` 事件标识符  
	返回：无  
	描述：等待事件被置位  
	> 参考：  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject  
________________________________________________________________________________
+ uint32_t event_timed_wait(event_t event， uint32_t timeout);  
	参数：`event` 事件标识符  
	参数：`timeout` 等待时间（毫秒）  
	返回：剩余等待时间，如果返回0则说明等待超时  
	描述：定时等待事件置位，如果等待时间超过`timeout`设定的时间则退出等待  
	> 参考：  
	> WIN32：https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject  

## 四、可选功能

可选功能的源码在`sources/opt/`目录，每个功能对应一个c文件和h文件，c文件是功能的实现，h文件是定义的接口。
要使用哪个功能就包含对应的头文件和源代码。

### 4.1 事件组

头文件: #include "event_group.h"


### 4.2 软定时器

头文件: #include "soft_timer.h"

### 4.3 消息队列

头文件: #include "msg_queue.h"

### 4.4 块队列

头文件: #include "blk_queue.h"

### 4.5 块内存池

头文件: #include "mem_pool.h"

### 4.6 字节流缓冲区

头文件: #include "byte_stream.h"

## 五、其它函数

### 5.1 通用链表

头文件: #include "list.h"

### 5.2 通用FIFO

头文件: #include "fifo.h"

## 六、硬件移植

KLite核心代码完全使用C语言实现，由于不同CPU平台的差异性，有一些功能依赖于目标CPU平台，这些无法统一实现的函数，可能需要使用汇编才能实现
这些函数的实现代码放在`sources/port/`目录。
________________________________________________________________________________
+ void cpu_sys_init(void);  
	描述：初始化与操作系统有关的功能，在`kernel_init()`阶段被调用  
	参数：无  
	返回：无  
________________________________________________________________________________
+ void cpu_sys_start(void);  
	描述：启动与操作系统有关的功能，在`kernel_start()`阶段被调用通常用于启动滴答时钟定时器，并打开中断滴答时钟使用硬件定时器中断，中断周期通常为1ms，
	也可以为其它任意值,只需要在中断服务程序中调用一次`kernel_tick(1)`即可这里的参数1则代表时钟周期，如果是10ms中断一次，则应该传入10作为参数。
	参数：无  
	返回：无  
________________________________________________________________________________
+ void cpu_sys_sleep(uint32_t time);  
	描述：实现低功耗的接口，操作系统休眠时调用  
	参数：`time`  可休眠的最长时间，单位毫秒  
	返回：无  
________________________________________________________________________________
+ void cpu_sys_enter_critical(void);  
	描述：实现操作系统进入临界区的接口，需要支持递归  
	参数：无  
	返回：无  
________________________________________________________________________________
+ void cpu_sys_leave_critical(void);  
	描述：实现操作系统退出临界区的接口，需要支持递归  
	参数：无  
	返回：无  
________________________________________________________________________________
+ void cpu_contex_switch(void);  
	描述：实现切换线程上下文的接口，需要使用可挂起的中断来实现  
	参数：无  
	返回：无  
________________________________________________________________________________
+ void *cpu_contex_init(void *stack, void *entry, void *arg, void *exit);
	描述：初始化线程栈的接口
	参数：`stack`    栈顶指针（KLite只适用于使用递减栈的CPU）
	参数：`entry`    线程入口函数地址
	参数：`arg`      线程入口函数参数
	参数：`exit`     线程出口函数地址
	返回：新的栈顶指针

## 七、设计思路

核心思想是通过调度器(sched.c)维护3个TCB链表（队列）：  
* 就绪链表(m_ready_list): 此版本使用了8条就绪表，每个表对应一个优先级，
	> 在之前的版本中，为了使代码更简单，使用的是一条双向链表，通过优先级进行降序排序，  
	> 优先级最高的在表头，但这种方案排序时间会随着优先级数量和线程数量上升，实时性较差。  
	> 为了适用于更多的场景中，此版本使用了8条就绪表，每个表对应一个优先级，这种方案无需排序，插入时间恒定，使整个系统有更强的实时性。
	> 8条就绪链表也就意味着系统最多支持8个优先级，经过反复推敲，8个优先级已经能应对绝大多数的项目需求了。

* 睡眠链表(m_sleep_list): 当线程休眠时，会加入此表，在每一次滴答周期内都会检查有否有线程休眠结束，将其移到就绪表中。

* 等待链表(tcb->list_wait): 用于线程同步互斥对象的阻塞。

每当需要切换线程时，就从就绪表中取出优先级最高的线程，将`sched_tcb_next`的值修改为新线程，调用硬件接口完成上下文切换。  
硬件切换接口`cpu_contex_switch()`挂起指定中断，在该中断服务程序中完成线程切换。  
切换过程：将当前上下文保存到`sched_tcb_now->sp`栈中，并更新sp值，  
然后从`sched_tcb_next->sp`中取出上下文，  
最后将`sched_tcb_next`赋值到`sched_tcb_now`。

## 八、结语

如果您在使用中发现任何BUG或者有好的改进建议，欢迎加入QQ群(317930646)或发送邮件至kerndev@foxmail.com
