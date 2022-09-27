---
marp: true
theme: gaia 
paginate: true

header: Prj2_SimpleKernel Part 1&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Project_2 &ensp;SimpleKernel
## ——Part1 &ensp;Design Review
### 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Question 1:
### How does switch_to work to support 
### context switch between two processes?

---
### switch_to
+ 功能简介：
  + 将当前进程`switchto_context_t`结构体对应寄存器存储至结构体
  + 从新进程结构体中加载对应的寄存器
  + 将新进程的PCB指针存至tp寄存器中，并跳转至新进程执行。
+ 实现细节：
  + 使用t0等作为中间寄存器，存储新旧进程结构体指针
  + 结构体相对PCB偏移、寄存器相对结构体偏移参见<asm/regs.h>
  + 将新进程PCB指针赋值保存至tp
  + 新进程的ra和sp初始化在`init_pcb_stack`中已完成
---
### switch_to
+ 方案对比：
  + 除上述方案外，依照原有代码中将SP寄存器先减后加的操作，另一种实现方法是通过sp作为中间寄存器，进行`switch_context_t`结构体存取
  + 该方案会在当前栈中存储当前进程寄存器，并更改`kernel_sp`指针。在新进程取寄存器值时，由于涉及sp改动，应当调整sp取值顺序至最后。最后将sp增大，一方面为了保持一致性，另一方面可以避免栈错误使用时更改结构体内容。
  + 两种方案均可通过仿真和上板测试。但从实现复杂度和维护`kernel_sp`指针不变的角度，选择前一种方案。
---
<!--_class: lead-->
# Question 2:
### How do you initialize PCB and kernel stack?

---
### init_pcb
+ 功能简介：
  + 初始化`ready_queue`
  + 对待测任务，设置`pid`和`status`，并初始化`kernel_sp`和`user_sp`
  + 调用`init_pcb_stack`进行内核栈的初始化布局
  + 将其加入`ready_queue`队列
  + 将`current_running`指向pcb0
  + 将`lock_time`置零（多锁机制处理进程饥饿问题）
  
---
### init_pcb
+ 实现细节：
  + 初始化队列时，方便起见，调用<list.h>中自定义的API将队列头尾置空
  + 进行`kernel_sp`和`user_sp`的初始化时，调用`alloc...Page`时返回的是栈底地址，因此需要额外加上`PAGE_SIZE`。方便后续栈的排布。
  + 进队列函数需要使用自己的API实现。`enqueue`函数中传入PCB指针，实际将指向PCB中的list成员的指针加至双向链表尾部。
  + pcb0在定义时已经初始化，可以直接使用
---
### init_pcb_stack
+ 功能简介：
  + 依次排布`regs_context_t`和`switchto_context_t`结构体
  + 修改`kernel_sp`及`switchto_context_t`结构体相关内容
+ 实现细节：
  + `kernel_sp`将作为`switch_to`存取内容的依据，此处将其置为已排布内容的底部，与`pt_switchtp`指针值相同
  + ra寄存器内容初始化为程序入口地址，即程序加载在内存的入口位置
  + sp寄存器内容初始化为`kernel_sp`，将内核栈剩余空白部分提供给程序使用
---
<!--_class: lead-->
# Question 3:
### What do you save in switchto_context_t?

---
### init_pcb_stack初始化
+ 如Q2所述，ra寄存器初始化为程序入口地址，sp寄存器初始化为已排布内容底部
+ 其余寄存器(s0-s11)置零
### switch_to切换
+ ra寄存器存储当前返回地址，即调用switch_to函数位置的下一条指令地址；sp寄存器存储当前栈指针。
+ s0-s11是API约定的被调用者保存寄存器，存储为当前值。
---
### 补充
+ 从一个用户进程切换到其他进程的流程是
  + kernel_yield() --> call_jmptab() --> do_scheduler() --> switch_to()
+ switch_to中保存当前进程的ra和sp寄存器内容。在该进程被重新调度执行时，会在上述函数调用返回过程中，进一步恢复。
  + ra寄存器恢复过程：
    + 依次恢复为调用函数的下一条指令，最终恢复为用户程序调用kernel_yield()前的ra
  + sp寄存器恢复过程：
    + 函数调用结束后进行栈还原，最终恢复为用户程序调用kernel_yield()前的sp
---
<!--_class: lead-->
# Question 4:
### Please explain the workflow of your 
### designed kernel scheduler (do_scheduler)

---
### do_scheduler
+ 功能简介：
  + 从ready_queue队头取出将切换的新进程
  + 允许满足条件的当前进程再次被调度
  + 将`current_running`指向新进程并修改状态
  + 调用`switch_to`进行上下文切换
+ 实现细节:
  + 当前进程不为pcb0且不为阻塞态时才允许再次被调度，需要修改状态，并入队ready_queue
  + 出入队列的操作进行封装实现，相关API见<sched.c>和<list.h>

---
### 队列相关API
+ 由于双向链表的成员仅含前后指针，因此`enqueue`和`dequeue`封装了PCB结构，实际使用`list_add_tail`或`list_del_head`将其中的list成员从尾部加入双向链表，或删除链表表头。
+ 为简化条件判断逻辑，使用`list_init`进行链表初始化，将其头尾指针置空。并在`list_add_tail`和`list_del_head`进行相应条件判断。
+ 相关API在task2阻塞时也有相关应用。
---
<!--_class: lead-->
# Question 5:
### When a process is blocked, how does 
### the kernel handle the blocked process?
---
<!--_class: lead-->
## Question 5_1:
### Where to place the PCB when 
### the process is blocked/unblocked?
---
### 单锁机制
+ 阻塞时，将该进程入队对应锁的block_queue
+ 取消阻塞时，将锁对应的block_queue队头出队直至清空，并将队头入队ready_queue。

### 多锁机制
+ 阻塞时（某些锁被占用），将该进程入队所有相关锁序列的block_queue。
+ 取消阻塞时（释放该任务对应的多个锁），将每个锁对应的block_queue逐个取出遍历，如相关锁均已空闲，则入队ready_queue，否则重新入队对应block_queue。

---
<!--_class: lead-->
## Question 5_2:
### Can your design support a 
### process acquire multiple locks? 

---
### 单锁机制
+ 锁的初始化：将key映射到锁的id，并根据该id进行锁的申请和释放
+ 锁的申请：需要循环进行，如果该锁未上锁，则将其上锁，并结束循环，开始执行用户程序；如果已上锁，则需调用`do_block`将该进程堵塞并进入阻塞队列。然后使用`do_scheduler`进行进程调度。（该进程解除阻塞并被再次调度时，由于仍在循环中，会再次申请锁）
+ 锁的释放：将该锁对应block_queue元素全部出队，并调用`do_unblock`方法恢复状态并入队`ready_queue`。

---
### 多锁机制
+ 由于考虑到一个key可能映射到多个锁id，而用户程序只能使用跳转表函数实现，不便于传参以及返回值获取。故在<list.h>定义lock_array结构体及相应全局变量lock_map进行实现。
+ 每个函数将根据其pid对应一个lock_array结构体，其中含有一个lock_hit数组，元素值为1表示该任务需要这把锁。lock_map是所有任务对应lock_array组成的数组。
```c
typedef struct lk_arr{
    int lock_hit[LOCK_NUM];
} lock_array;

lock_array lock_map[TASK_MAXNUM]; //对应不同pid的命中锁状态
```
---
### 多锁机制
+ 锁的初始化：根据key获取该进程所需锁，并获取当前进程pid。将lock_map[pid]相应位置置1。
+ 锁的申请：需要循环进行。首先根据lock_map获取该进程相关的锁序列，并对其进行遍历。如果均未上锁，则将序列中的锁均上锁，并结束循环，开始执行用户程序；如果有某把锁上锁，则对序列中的每把锁，均调用`do_block`将其加入阻塞队列，最终使用`do_scheduler`进行进程调度。
+ 锁的释放：首先获取进程相关锁序列。将相关序列中每个锁对应的block_queue逐个取出遍历，如相关锁均已空闲，则入队ready_queue，否则重新入队对应block_queue。最后将序列中的锁标记为UNLOCKED

---
### 多锁机制
+ 锁释放实现细节
  + 对每个队列，由于可能队头拿出不满足条件再从队尾插入，因此初始头部需要拿出作为终止条件。
  + 相关序列中的不同锁的block_queue可能有重复任务，但不应当重复进入ready队列。因此设置used[TASK_MAX_NUM]。对成功入队ready_queue的任务使用used记录，之后遍历如果为used，可以直接出队。
  + 对每次队头对应的pcb，依次检查其他序列外锁的阻塞状态，如果仍是阻塞，block_queue出队后需要再次入队。否则，出队后可以使用`do_unblock`改变状态并入队ready_queue
  
---
### 多锁机制
+ 锁释放实现细节
  + 由于释放的锁序列和被其堵塞任务的锁序列可能只部分重叠，因此在被堵塞任务取消阻塞、被调度并最终释放相关锁时，阻塞队列中含有该任务本身，可以直接出队。
  + 例如，任务A占用锁1、2，任务B占用锁2、3。当A执行时，B由于锁2被占用堵塞，加入锁2、3的block_queue。A释放锁，并遍历到锁2的block_queue时，由于任务B未在相关锁序列中的锁3未被阻塞，故将B从锁2队列出队，并调度执行，占用锁2、3。当B释放锁时，需要遍历锁2、3的阻塞队列，本身仍在锁3的阻塞队列中，需要直接出队。

---
<!--_class: lead-->
## Question 5_3:
### Can two or more processes 
### acquire the same lock?

---
### 进程饥饿/锁的不公平分配
+ 对于单锁机制，或者多锁机制下不同任务的相关锁序列完全重合的情形，只要固定阻塞序列的遍历顺序，就可以保证多个任务的执行轮转顺序一致，锁公平分配。
+ 针对多锁机制下不同任务的相关锁序列只是部分重合时，可能由于block_queue进入ready_queue顺序产生进程饥饿。例如，lock1和mylock占用锁1、2，lock2占用锁2、3。当lock1占用，将另外二者堵塞，由于先遍历锁1阻塞度列，mylock先释放。mylock执行时，将另外二者堵塞，lock1先释放。上述过程循环进行，导致lock2无法获取锁，进程饥饿。

---
### ROB (Re-order Buffer) 重排缓冲区
+ 在PCB结构体中增加`lock_time`成员，表示该进程获取锁的次数，并在`init_pcb`中初始化为0
+ 在`do_mutex_lock_acquire`函数中，如果一个进程获取到锁序列，则将其`lock_time`增大
+ 在`do_mutex_lock_release`函数中，将所有欲调用`do_unblock`进入就绪队列的任务传入ROB。待相关锁序列的阻塞队列均遍历完毕后，对ROB进行升序重排，获取锁次数越少，优先级越高。重排完毕后，再调用`do_unblock`入队。
+ 通过该方法，可以实现锁在各进程的公平分配，解决进程饥饿问题。