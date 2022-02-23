# KLite 参考手册
	更新日期: 2022.02.22

## 一、简介

**KLite** 是由个人开发者编写的嵌入式操作系统内核，创建于2015年5月6日，并以MIT协议开放源代码。

KLite的定位是一款入门级的嵌入式实时操作系统内核，以简洁易用为设计目标，旨在降低学习嵌入式RTOS的入门难度。代码干净工整，架构清晰，模块化设计简单明了，不使用条件编译，代码可读性强。API函数简单易用、移植方法简单、无需复杂的配置和裁减，可能是目前最简洁易用的RTOS。
___

**功能特性：**

- 资源占用极小（ROM：2KB，RAM：0.5KB）
- 优先级抢占
- 时间片抢占（相同优先级）
- 支持创建相同优先级的线程
- 丰富的线程通信机制  
- 动态内存管理
- 多编译器支持（GCC,IAR,KEIL）

___

**作者简介：** 蒋晓岗，男，处女座，2013年毕业于成都信息工程大学计算机科学与技术专业，从事嵌入式软件开发工作近十年，有代码强迫症，追求精简又整齐的编码风格，擅长使用基于ARM内核的微控制器。

## 二、开始使用

KLite目前支持ARM7、ARM9、Cortex-M0、Cortex-M3、Cortex-M4、Cortex-M7内核，

若你的MCU是基于以上平台如：全志F1C100S、STM32、NRF51/52等，那么可以参考相关例程。

其实对于Cortex-M架构的MCU，实际上只需要修改cmsis.c里面的#include即可，比如#include "stm32f10x.h"。

	由于KLite源码不使用条件编译，因此可以直接预编译kernel源码为kernel.lib，并保留kernel.h
    可以有效减少重复编译的时间。

main.c的推荐写法如下:
```
//只需要包含这一个头文件即可
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

## 三、 核心功能

核心功能的源码在sources/kernel/目录，是KLite最核心的部分。
使用这些功能，只需要包含"kernel.h"即可。

### 3.1 内核管理

#### void kernel_init(void *heap_addr, uint32_t heap_size)

**参数：**

`heap_addr` 动态分配起始地址

`heap_size` 动态分配内存大小

**返回：**

无

**功能描述：**

用于内核初始化。在调用内核初始化时需保证中断处于关闭状态，此函数只能执行一次，在初始化内核之前不可调用内核其它函数。

----
#### void kernel_start(void)

**参数：**

无

**返回：**

无

**功能描述：**

用于启动内核。此函正常情况下不会返回，在调用之前至少要创建一个线程。

----
#### uint32_t kernel_version(void)

**参数：**

无

**返回：**

KLite版本号，BIT[31:24]主版本号，BIT[23:16]次版本号，BIT[15:0]修订号
**功能描述：**

此函数可以获取KLite的版本号

----
#### uint32_t kernel_time(void)

**参数：**

无

**返回：**

系统运行时间(毫秒)

**功能描述：**

此函数可以获取内核从启动到现在所运行的总时间。
> 此函数没有命名为kernel_get_time()，一方面是为了缩短函数名看起来更整齐，另一个原因是因为time只能读不能写，不需要区分get和set。

----
#### void kernel_idle(void)

**参数：**

无

**返回：**

无

**功能描述：**

处理内核空闲事务，回收线程资源。此函数不会返回，需要单独创建一个用户线程，并直接调用。
> 由用户创建空闲线程是为了实现灵活配置空闲线程的stack大小，避免使用宏定义进行配置。

----
#### uint32_t kernel_idle_time(void)
**参数：**

无

**返回：**

系统空闲时间(毫秒)

**功能描述：**

获取系统从启动到现在空闲线程运行的总时间，可使用此函数和kernel_time一起计算CPU占用率。

----
#### void kernel_tick(uint32_t time)

**参数：**

滴答周期(毫秒)

**返回：**

无

**功能描述：**

此函数不是API接口，它应该由CPU的滴答时钟中断程序调用。

> 此函数为KLite系统提供时钟源。在这里把滴答时间转为毫秒，应用层就不必每次都要用宏定义来进行毫秒转滴答，可以简化接口调用。

### 3.2 内存管理

----
#### void *heap_alloc(uint32_t size)

**参数：**

`size` 要申请的内存大小

**返回：**

申请成功返回内存指针，申请失败返回NULL

**功能描述：**

从堆中申请一段连续的内存，功能和标准库的malloc()一样。

> 严谨的代码在调用此函数时应该检查返回值是否为NULL，但每次alloc都要写代码检查返回值可能有点很难受，所以此函数在返回NULL之前会调用一个HOOK函数，原型为`void heap_overflow(void);`
> 对于嵌入式系统来说申请内存失败是严重错误，所以只需要在heap_overflow()函数中处理错误就足够了。

----
#### void heap_free(void *mem)

**参数：**

`mem` 要释放的内存指针

**返回：**

无

**功能描述：**

释放由heap_malloc申请的内存，功能和标准库的free()一样。

----
#### void heap_usage(uint32_t *used, uint32_t *free)

**参数：**

`used` 输出已使用内存数量(字节)，此参数可以为NULL。

`free` 输出空闲内存数量(字节)，此参数可以为NULL。

**返回：**

无

**功能描述：**

获取内存用量信息，可使用此函数关注系统内存消耗，以适当调整内存分配。

### 3.3 线程管理

#### thread_t thread_create(void(*entry)(void*), void *arg, uint32_t stack_size)

**参数：**

`entry` 线程入口函数

`arg` 线程入口函数的参数

`stack_size` 线程拥有的栈大小（字节），如果该值为0则使用系统默认值：1024字节

**返回：**

成功返回线程句柄，失败返回NULL

**功能描述：**

创建新线程，并加入就绪队列，新线程的优先级为默认值：`THREAD_PRIORITY_NORMAL`。

> 系统自动为新线程分配内存和栈空间，如果栈设置太小运行过程中可能会产生栈溢出。

----
#### void thread_delete(thread_t thread)

**参数：**

`thread`  被删除的线程标识符

**返回：**

无

**功能描述：**

删除线程，并释放内存。该函数不能用来结束当前线程。如果想要结束当前线程请使用`thread_exit()`或直接使用return退出主循环。

> 不推荐直接删除线程，可能会造成系统不稳定。因为被删除线程可能持有了`mutex`或其它临界资源未释放，需考虑清楚。

-----
#### void thread_set_priority(thread_t thread, uint32_t prio)

**参数：**

`thread` 线程标识符

`prio` 新的优先级
> `THREAD_PRIORITY_HIGHEST` 最高优先级
> `THREAD_PRIORITY_NORMAL` 默认优先级
> `THREAD_PRIORITY_LOWEST` 最低优先级

**返回：**

无

**功能描述：**

重新设置线程优先级，立即生效。

-----
#### uint32_t thread_get_priority(thread_t thread)

**参数：**

`thread` 线程标识符

**返回：**

目标线程的优先级

**功能描述：**

获取指定线程的优先级

----
#### thread_t thread_self(void)

**参数：**

无

**返回：**

调用线程的标识符。

**功能描述：**

用于获取当前线程标识符。

----
#### void thread_sleep(uint32_t time)
**参数：**

`time` 休眠时间（毫秒）

**返回：**

无

**功能描述：**

将当前线程休眠一段时间，释放CPU控制权

-------------------------------------------------------------------------------
#### void thread_yield(void)
**参数：**

无

**返回：**

无

**功能描述：**

使当前线程立即释放CPU控制权，并进入就绪队列。

-------------------------------------------------------------------------------
#### void thread_exit(void)
**参数：**

无

**返回：**

无

**功能描述：**

退出当前线程，此函数不会立即释放线程占用的内存，需等待系统空闲时释放。

----
#### void thread_suspend(void)
**参数：**

无

**返回：**

无

**功能描述：**

挂起当前线程。

> 此函数故意设计为没有参数，不能挂起指定线程，原因和不推荐使用`thread_delete`一样。

-------------------------------------------------------------------------------
#### void thread_resume(thread_t thread)

**参数：**

`thread` 线程标识符

**返回：**

无

**功能描述：**

恢复被挂起的线程。

> 如果指定线程没有被挂起，可能会导致意外行为。

----
#### uint32_t thread_time(thread_t thread)

**参数：**

`thread` 线程标识符

**返回：**

指定线程运行时间（毫秒）。

**功能描述：**

获取线程自创建以来所占用CPU的时间。在休眠期间的时间不计算。

> 可以使用此函数来监控某个线程的CPU占用率。

#### 3.1.4 互斥锁
-------------------------------------------------------------------------------
	mutex_t mutex_create(void)
	功能：创建一个新的互斥锁
	参数：无
	返回：成功返回互斥锁标识符，失败返回NULL
	备注：互斥锁支持递归调用。
-------------------------------------------------------------------------------
	void mutex_lock(mutex_t mutex)
	功能：将mutex指定的互斥锁标记为锁定状态
	参数：mutex   互斥锁标识符
	返回：无
	备注：如果mutex指向的互斥锁已被其它线程锁定，则调用线程将会被阻塞，直到另一个线程释放这个互斥锁。
-------------------------------------------------------------------------------
	bool mutex_try_lock(mutex_t mutex)
	功能：尝试将mutex指定的互斥锁标记为锁定状态
	参数：mutex   互斥锁标识符
	返回：锁定成功，则返回true;锁定失败则返回false。
	备注：此函数是mutex_lock的非阻塞版本。
-------------------------------------------------------------------------------
	void mutex_unlock(mutex_t mutex)
	功能：释放由参数mutex指定的互斥锁
	参数：mutex   互斥锁标识符
	返回：无
	备注：mutex_lock和mutex_unlock必须成对出现。
-------------------------------------------------------------------------------
	void mutex_delete(mutex_t mutex)
	功能：删除一个互斥锁，释放内存
	参数：mutex   被删除的互斥锁标识符
	返回：无
	备注：在删除互斥锁的时候不会检查是否有线程拥有这个互斥锁。因此在删除之前请确认没有线程在使用。

#### 3.1.5 信号量
-------------------------------------------------------------------------------
	sem_t sem_create(uint32_t value)
	功能：创建一个信号量对象。
	参数：value    信号量初始值
	返回：成功返回信号量标识符，失败返回NULL。
	备注：

-------------------------------------------------------------------------------
	void sem_delete(sem_t sem)
	功能：删除对象，并释放内存。
	参数：sem   信号量标识符。
	返回：无。
	备注：在没有线程使用它时才能删除，否则可能产生未知的异常。

-------------------------------------------------------------------------------
	void sem_post(sem_t sem)
	功能：信号量计数值加1。
	参数：sem   信号量标识符。
	返回：无。
	备注：如果有线程在等待信号量，此函数会唤醒其中一个线程。

-------------------------------------------------------------------------------
	void sem_wait(sem_t sem)
	功能：信号量计数值减1。
	参数：sem   信号量标识符。
	返回：无。
	备注：如果当前信号量计数值为0，则线程阻塞，直到计数值大于0。

-------------------------------------------------------------------------------
	uint32_t sem_timed_wait(sem_t sem, uint32_t timeout)
	功能：信号量计数值减1。
	参数：sem     信号量标识符。
	参数：timeout 超时时间，单位为毫秒。
	返回：剩余等待时间，如果返回0则说明等待超时。
	备注：如果当前信号量计数值为0，则线程阻塞，直到计数值大于0，或者阻塞时间超过timeout指定的时间。

-------------------------------------------------------------------------------
	uint32_t sem_value(sem_t sem);
	功能：返回信号量计数值。
	参数：sem   信号量标识符。
	返回：信号量计数值
	备注：
	
#### 3.1.6 条件变量
-------------------------------------------------------------------------------
	cond_t cond_create(void);
	功能：创建条件变量。
	参数：无
	返回：条件变量标识符
-------------------------------------------------------------------------------
	void cond_delete(cond_t cond);
	功能：删除条件变量。
	参数：cond 条件变量标识符
	返回：无
-------------------------------------------------------------------------------
	void cond_signal(cond_t cond);
	功能：唤醒一个被条件变量阻塞的线程。
	参数：cond 条件变量标识符
	返回：无
-------------------------------------------------------------------------------
	void cond_broadcast(cond_t cond);
	功能：唤醒全部被条件变量阻塞的线程。
	参数：cond 条件变量标识符
	返回：无
-------------------------------------------------------------------------------
	void cond_wait(cond_t cond, mutex_t mutex);
	功能：等待条件变量被唤醒。
	参数：cond 条件变量标识符
	参数：mutex 互斥锁标识符
	返回：无
-------------------------------------------------------------------------------
	uint32_t cond_timed_wait(cond_t cond, mutex_t mutex, uint32_t timeout);
	功能：定时等待条件变量被唤醒。
	参数：cond 条件变量标识符
	参数：mutex 互斥锁标识符
	参数：timeout 超时时间，单位为毫秒。
	返回：剩余等待时间，如果返回0则说明等待超时。

#### 3.1.7 事件标志
-------------------------------------------------------------------------------
	event_t event_create(bool auto_reset)
	功能：创建一个新的事件对象。
	参数：auto_reset 是否自动复位事件标志
	返回：创建成功返回事件标识符，失败返回NULL。
	备注：无
-------------------------------------------------------------------------------
	void event_delete(event_t event)
	功能：删除事件对象，并释放内存。
	参数：event   事件标识符。
	返回：无。
	备注：在没有线程使用它时才能删除，否则可能产生未知的异常。
-------------------------------------------------------------------------------
	void event_set(event_t event)
	功能：标记事件为置位状态。
	参数：event   事件标识符。
	返回：无。
	备注：如果auto_reset为true，那么会唤醒等待队列中的第1个线程，并且将事件自动复位。如果auto_reset为false，那么会唤醒等待队列中的所有线程，并且将事件保持置位。
-------------------------------------------------------------------------------
	void event_reset(event_t event)
	功能：标记事件为复位状态。
	参数：event   事件标识符。
	返回：无。
	备注：此函数不会唤醒任何被event挂起的线程。
-------------------------------------------------------------------------------
	void event_wait(event_t event)
	功能：等待事件置位。
	参数：event   事件标识符。
	返回：无。
	备注：无
-------------------------------------------------------------------------------
	uint32_t event_timed_wait(event_t event， uint32_t timeout)
	功能：定时等待事件置位，如果等待时间超过timeout设定的时间则退出等待。
	参数：event   事件标识符。
	参数：timeout 等待时间，单位为毫秒。
	返回：剩余等待时间，如果返回0则说明等待超时。
	备注：无

### 3.2 可选功能
TODO

### 3.3 移植接口
KLite核心代码完全使用C语言实现，由于不同CPU平台的差异性，有一些功能依赖于目标CPU平台，这些无法统一实现的函数，可能需要使用汇编才能实现。

-------------------------------------------------------------------------------
	void cpu_sys_init(void);
	描述：初始化与操作系统有关的功能，在kernel_init阶段被调用。
	参数：无
	返回：无
-------------------------------------------------------------------------------
	void cpu_sys_start(void);
	描述：启动与操作系统有关的功能，在kernel_start阶段被调用。通常用于启动滴答时钟定时器，并打开中断。滴答时钟使用硬件定时器中断，中断周期通常为1ms，也可以为其它任意值,只需要在中断服务程序中调用一次kernel_tick(1)即可。这里的参数1则代表时钟周期，如果是10ms中断一次，则应该传入10作为参数。
	参数：无
	返回：无
-------------------------------------------------------------------------------
	void cpu_sys_sleep(uint32_t time);
	描述：实现低功耗的接口，操作系统休眠时调用。
	参数：time  可休眠的最长时间，单位毫秒。
	返回：无
-------------------------------------------------------------------------------
	void cpu_sys_enter_critical(void);
	描述：实现操作系统进入临界区的接口，需要支持递归。
	参数：无
	返回：无
-------------------------------------------------------------------------------
	void cpu_sys_leave_critical(void);
	描述：实现操作系统退出临界区的接口，需要支持递归。
	参数：无
	返回：无
-------------------------------------------------------------------------------
	void cpu_contex_switch(void);
	描述：实现切换线程上下文的接口，需要使用可挂起的中断来实现。
	参数：无
	返回：无
-------------------------------------------------------------------------------
	void *cpu_contex_init(void *stack, void *entry, void *arg, void *exit);
	描述：初始化线程栈的接口。
	参数：stack    栈顶指针（KLite只适用于使用递减栈的CPU）
	参数：entry    线程入口函数地址
	参数：arg      线程入口函数参数
	参数：exit     线程出口函数地址
	返回：新的栈顶指针

## 四、设计解读
TODO

---
如果你在使用中发现任何BUG或者改进建议，欢迎加入QQ群(317930646)或发送邮件至kerndev@foxmail.com
