---
marp: true
theme: gaia 
paginate: true

header: Prj2_SimpleKernel Part 2&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Project_2 &ensp;SimpleKernel
## ——Part2 &ensp;Design Review
### 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Question 1:
### Which registers do you need to initialize
### for setting up and handling exception? 
### How do you set their values?

---
### 全局相关寄存器(trap.S)
+ 相关寄存器
  + STVEC：设置中断处理函数入口地址及访问方式
  + SSTATUS: 全局使能中断，`SIE`位置置1
  + SIE：使能具体中断，对应位置置1
+ 设置方式
  + 将写入值存入中间通用寄存器
  + 通过`csrw`指令写CSR寄存器

---
### PCB相关寄存器
+ regs_context_t结构体
  + sp设置为user_sp：存储用户栈，供上下文恢复使用
  + tp设置为指向该PCB结构体的指针：在上下文保存和恢复中保持不变
  + sepc设置为函数入口地址：异常处理结束后`sret`指令返回地址
  + sstatus初始化SPP(0)和SPIE(1)：初始化特权级和中断使能状态，通过恢复上下文使用户程序进入用户态并允许中断
+ 设置方式
  + ret_from_exception中恢复上下文时存至对应寄存器
---
### PCB相关寄存器
+ switchto_context_t结构体
  + ra设置为ret_from_exception函数地址：切换进程后，可恢复上下文和用户态
  + sp设置为kernel_sp：存储内核栈，供进程切换和上下文恢复使用
+ 设置方式
  + switch_to中切换进程时存至对应寄存器

---
<!--_class: lead-->
# Question 2:
### Please describe the procedure 
### and possible functions you will 
### implement to support syscall

---

![bg 82%](prj2.png)

---
### 调用流程
1. 进程由于`ecall`系统调用或时钟中断陷入内核态，到达异常处理函数入口exception_handler_entry
2. 保存上下文后，将跳转至interrupt_helper例外分发函数，判断例外类型并调用相关处理程序
3. 对于系统调用，handler_syscall将调用相关程序进行处理，如调度，申请锁等；
4. 对于时钟中断，handler_irq_timer将重置定时器并进行调度处理
5. 上述处理完毕后，将返回至exception_handler_entry并进一步跳转到ret_from_exception恢复上下文
6. 最后将通过`sret`指令返回用户态，继续运行用户程序

---
### 补充：第一次调度
  + 由于系统调用和时钟中断都是在用户态实现的，因此应当第一次调度用户任务应当让其进入用户态。在用户态运行过程中再进行系统调用或时钟中断
  + 第一次通过do_scheduler调度至该用户任务时，switch_to将返回至ret_from_exception进行上下文恢复。
  + 根据PCB内核栈初始化情形，可设置用户任务状态寄存器及中断使能寄存器，`sret`指令使该进程进入用户态、使能中断、并跳转至程序入口。

---
### 关键函数列表
+ init_syscall：初始化系统调用表，与相应函数绑定
+ init_exception：初始化例外表和中断表，与相应处理函数绑定
+ setup_exception：初始化异常处理入口，使能全局中断
+ invoke_syscall：发起系统调用并进行传参
+ exception_handler_entry：中断处理函数入口，保存上下文
+ interrupt_helper：判断例外类型，调用相应函数
+ handle_syscall：根据系统调用号传参并调用对应函数
+ handle_irq_timer：处理时钟中断
+ ret_from_exception：恢复上下文，返回用户态

---
### init_syscall
+ 实现方法与跳转表类似，但该数组为内核数据段中的全局变量，并非固定地址，用户程序无法直接访问（DASIC功能）

### init_exception
+ 将系统调用与时钟中断的处理函数与对应例外类型绑定，其余未定义类型初始化为handle_other，以方便检错

### setup_exception
+ 设置异常处理函数入口，并使能全局中断

---
### invoke_syscall
+ 进行传参，使用`ecall`指令进入内核态并跳转至异常处理函数入口
+ 所传参数将在上下文保存时存至内核栈，供handle_syscall使用
+ 传参方式详见问题3

### exception_handler_entry
+ 进行上下文保存，调用interrupt_helper处理例外，最终返回ret_from_exception
+ 根据函数调用API，应当将interrupt_helper所需参数分别存至a0-a2

---
### interrupt_helper
+ 根据scause可判断例外类型：是否中断，具体类型等
+ 根据例外/中断和具体类型调用相应处理函数

### handler_syscall
+ 根据系统调用号传参并调用对应函数
+ 修改epc至下一条指令位置，防止陷入死循环
+ 所需参数可从内核栈相应位置获得，在异常处理入口保存上下文时存入内核栈，与invoke_syscall中填入参数的寄存器位置相一致
+ 调用结果应当存入内核栈，位置同样与invoke_syscall相一致

---
### handler_irq_timer
+ 调用check_sleeping唤醒睡眠结束进程（task3未实现时间中断时在调度时完成）
+ 重置定时器，制造下一次时间中断
+ 进行调度，切换至新进程

### ret_from_exception
+ 恢复上下文，使用`sret`指令返回用户态，并从例外发生位置继续执行

---
<!--_class: lead-->
# Question 3:
### How do you pass parameters 
### when implementing syscall?

---
```c
static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4)
{
    /* TODO: [p2-task3] implement invoke_syscall via inline assembly */
    asm volatile("nop");
    // register type var-name asm(reg)
    //其中type为变量类型 var-name为变量名称 reg为欲访问的寄存器
    //该语法允许变量与寄存器对应
    register uint64_t a0 asm("a0") = (uint64_t)(arg0);
    register uint64_t a1 asm("a1") = (uint64_t)(arg1);
    register uint64_t a2 asm("a2") = (uint64_t)(arg2);
    register uint64_t a3 asm("a3") = (uint64_t)(arg3);
    register uint64_t a4 asm("a4") = (uint64_t)(arg4);
    register uint64_t a7 asm("a7") = (uint64_t)(sysno);

    //memory 修饰用于保持顺序以及直接从内存读取
    asm volatile(
        "ecall\n\t"
        : "+r"(a0)
        : "r"(a1),"r"(a2),"r"(a3),"r"(a4),"r"(a7)
        : "memory"
    );

    return a0;
}
```
---
+ 寄存器绑定语法
  + register type var-name asm(reg)
  + 其中type为变量类型 var-name为变量名称 reg为欲访问的寄存器。该语法允许变量与寄存器对应

+ RISC-V 参数约定
  + 根据API约定，系统调用号应存于a7寄存器，其余参数依次存入a0-a4寄存器。返回值存于a0寄存器。此处实现和API约定一致。

+ 内联汇编语法
  + "+r"表示a0寄存器将同时作为输入和输出寄存器，
  + "memory"修饰保证相关数据直接从内存读取，而非寄存器缓存值
---
<!--_class: lead-->
# Question 4:
### What are user-level and kernel-level 
### stacks used for? 
### Are they used during handling
### exception? If so, how?

---
### 用户栈
+ 存储用户程序运行过程中所用到的局部变量等，供用户程序在用户态的正常运行使用
+ 指示当前用户栈指针的sp寄存器将会在触发例外时，通过保存上下文保存至内核栈中，在中断返回恢复上下文时进行恢复。
+ 在处理例外的过程中，仅涉及用户栈指针的保存、恢复或切换，并未涉及到用户栈中的具体内容。

---
### 内核栈
+ 存储用户态和内核态切换、进程切换时所需寄存器；存储内核程序运行过程中所用到的局部变量等，供内核程序运行使用
+ regs_context_t结构体中存储用户态和内核态切换中所需保存和恢复的寄存器。例如CSR寄存器用于进行特权级、中断使能状态等的切换；sp和ra用于存储用户程序继续运行的指令地址和用户栈；a0-a7等寄存器用于系统调用的传参和返回。
+ switchto_context_t结构体存储内核态进程切换时所需保存和恢复的寄存器，例如ra存储进行切换后新进程将返回的函数入口，sp存储切换新进程后对应的内核栈指针。
+ 其余部分供内核程序运行时使用（保存上下文后sp移至对应位置）

---
<!--_class: lead-->
# Question 5:
### When do you wake up the sleeping 
### process in Task 3 and 4?

---
### TASK3
+ 由于task3仍未实现时钟中断，只能选择在用户程序切换到内核态时进行睡眠任务的检查。可以将其放置在handle_syscall的系统调用处理函数中，也可以放在具体的处理函数中，如do_scheduler中。
+ 考虑到此时只有sys_yield系统调用，即对应内核中的do_scheduler函数才可进行进程的切换，为减少每次系统调用都检查睡眠队列的开销，将check_sleeping函数放在do_sheduler函数中进行调用
+ 该方法的一个问题是，如果用户程序的sys_yield频率较低，睡眠队列检查不够及时，可能导致唤醒时间不同的进程唤醒时间相近，导致其执行顺序与预期不符，影响函数功能实现

---
### TASK4
+ TASK4中实现了时钟中断，因此可以将睡眠队列检查放置在时间中断处理时进行
+ 该方法可以保证对睡眠队列进行较固定频率的检查，避免TASK3所采取方式中，由不同进程sys_yield频率不同带来的检查频率差异和检查不及时带来的功能错误。

---
<!--_class: lead-->
# Thread-related Func
---
### thread_create
+ 参数列表：ptr_t funcaddr, void *arg, ptr_t rc_funcaddr
  + 分别表示线程对应功能函数入口，对应参数，回收函数入口地址
+ 线程tcb初始化：
  + pid设置同主线程（进程）一致，tid设为对应传入参数以进行区分
  + tcb_num表示所占用的tcb块，便于线程回收时tcb块回收
  + 用户栈和内核栈均可重新分配
  + regs_context_t结构体中，epc设置为功能函数入口，a0设置为对应参数（传入参数为tid的地址），ra设置为回收函数地址
  + 其余与进程初始化类似

---
### thread_recycle
+ 相关分配函数的修改
  + 为实现线程回收（用户栈、内核栈、线程块回收），需要修改相应的分配函数。
  + 对于内存和tcb块的回收，均使用数组进行实现。地址数组提供了可用列表，标记数组提供了每个元素的状态（0表示空闲，1表示被占用）。
+ 回收
  + 对于用户内存、内核内存、tcb块，只需将相应占用标记置0，该位置即可被再分配。新分配进程将负责相应的初始化。
  + 该线程需要切换至其他进程或线程，且该tcb指针不再入队

---
<!--_class: lead-->
# Question and Thought

---
### 定时器设置
+ 当定时器设置中断频率过高，一方面时钟中断处理和进程切换开销大，多线程执行效率低；另一方面当定时器倒计时为0时内核处理还未结束，由于内核态屏蔽该中断，导致了中断失效，无法进行调度。
  + 将重置定时器移至handler_syscall结尾可避免内核处理未结束问题
+ 当定时器设置中断频率过低，线程切换频率过低，无法充分利用线程
+ 最后选择将下一次中断来临间隔设置为`timebase/20000`，可以在qemu和上板都取得较为合适的运行效果。偶然会遇到运行暂停的现象，按`ctrl+A`可继续。
+ 更优化的定时器设置策略？