---
marp: true
theme: gaia 
paginate: true

header: Prj3_InteractiveOS &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp; 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Project_3 &ensp;InteractiveOS
## —— Design Review
### 游昆霖 2020K8009926006

---
<!--_class: lead-->
# Question 1: 
### Show example code about 
### spawn, kill, wait, and exit?

---
### Spawn / Exec
#### 用户部分
```c
pid_t exec(){
    pid_t pid;
    if(isbackground==1){
        pid = sys_exec(argv[0],argc,argv);
        if(pid==0) printf("Error: %s: no such file\n",argv[0]);
        else printf("Info : execute %s successfully, pid = %d\n",argv[0],pid);
    }
    else{
        pid = sys_exec(argv[0],argc,argv);
        if(pid==0) printf("Error: %s: no such file\n",argv[0]);
        else sys_waitpid(pid);
    }
    return pid;
}
```
---
#### 内核部分
```c
pid_t do_exec(char *name, int argc, char *argv[]){
    char *shellptr = "shell";
    if(strcmp(name,shellptr)==0){
        printk("shell exists.\n");
        return 0;
    }
    pid_t pid = init_pcb(name,argc,argv);
    return pid; //不存在时返回0
}
```
#### 实现要点
+ 对加载入镜像的用户程序，不允许shell再次被用户创建
+ 对于不存在或非法的程序，返回pid为0，并进行报错

---
#### init_pcb更改 —— shell和其他用户程序的异同
+ 区别
  + 针对shell程序，完成了ready_queue和sleep_queue的初始化，设置mask为3，父进程ppid为0，不需参数排布
  + 针对其他用户程序，ppid置为父进程pid，继承mask，需参数排布

+ 共同步骤
  + 寻找对应程序入口，占据空闲PCB块
  + PCB块相关量的初始化
  + 调用`init_pcb_stack`进行内核栈的初始化
  + 入队ready_queue

---
### wait
#### 内核部分
```c
int do_waitpid(pid_t pid){
    if(pcb_flag[pid-1] == 1){
        do_block(&(current_running->list),&(pcb[pid-1].wait_queue));
        do_scheduler();
        return pid;
    }
    else
    {
        return 0;
    }
}
```

---
### exit
#### 内核部分
```c
void do_exit(void){
    pid_t pid = current_running->pid;
    pcb_recycle(pid);
    current_running->status = TASK_EXITED;
    do_scheduler();
}
```
#### 使用方式
+ 在`crt0`中，将用户程序的返回地址设置为`sys_exit`，从而实现进程的自动回收

---
### kill
#### 用户部分
```c
void kill(){
    int killpid = atoi(argv[0]);
    int res = sys_kill(killpid);
    if(res==1)
        printf("Info : kill successfully\n");
    else 
        printf("Error: the pid is no allocated\n");
}
```

---
#### 内核部分
```c
int do_kill(pid_t pid){
    if(pid == current_running->pid){
        do_exit();
        return 1;
    }
    else if(pcb_flag[pid-1] == 1){
        pcb[pid-1].status = TASK_EXITED;
        pcb_recycle(pid);
        return 1;
    }
    else 
        return 0;
}
```
+ 对于杀死其他进程，减少了不必要的调度步骤

---
### 进程的回收
#### 实现要点
1. 杀死子进程
2. 回收内存
3. 释放锁队列和等待队列
4. 释放创建的屏障、条件变量
5. 释放占用的信箱（减少访问量）
6. 取消PCB块占用标记
+ 由于涉及后续问题，方案分析详见[讨论——进程回收](#讨论——进程回收)
  
---
<!--_class: lead-->
# Question 2:
### How do you handle the case when 
### killing a task meanwhile it holds a lock?

---
#### 实现要点
+ 在锁被占用时，标记`host_id`为占用者的pid
+ 退出/杀死进程时，释放资源，遍历锁队列，发现其`host_id`为欲杀死进程的pid时，将该锁堵塞队列释放。

#### 方案分析
+ 由于一把锁至多被一个进程持有，而一个进程可以获取多把锁。由于当前锁数组较小，通过对锁加标记并遍历的方式实现简单，开销小。
+ 当可用锁数组较大时（远大于一个进程持有的锁数量），则应当在PCB块中记录该进程申请到的锁，从而进行释放，以降低遍历开销
  
---
<!--_class: lead-->
# Question 3:
### How do you handle condition variables, and barrier? 
### What to do if timer interrupt occurs?

---
### Barrier
+ 结构定义
```c
typedef struct barrier
{
    int total_num;
    int wait_num;
    list_head wait_queue;
    int used; 
    int host_id;
} barrier_t;
```
+ 全局初始化：清空相关量

---
+ 屏障初始化
  + 根据key获取对应屏障号
  + 设置屏障总数total_num
  + 标记占用并记录创建者id

+ 等待
  + 进程调用barriar_wait时增加等待量wait_num，堵塞该进程并调度
  + 当等待量与总数相等时，释放堵塞队列

+ 销毁（部分分析见[讨论——同步信号回收](#讨论——同步信号回收)）
  + 通过系统调用实现，或创建该屏障的进程结束/杀死后，自动销毁
  + 释放堵塞队列，并重置相关量
  
---
### Condition Variables
+ 结构定义
```c
typedef struct condition{
    list_head wait_queue;
    int used;
    int host_id;
} condition_t;
```
+ 全局初始化：清空相关量
+ 条件变量初始化
  + 根据key获取对应标号
  + 标记占用并记录创建者id
  
---
+ 等待
  + 堵塞该进程，释放锁（旧进程失去锁）
  + 调度，并重新上锁（新进程获得锁）

+ 唤醒
  + signal只唤醒一个，broadcast唤醒所有，唤醒进程加入就绪队列

+ 销毁（部分分析见[讨论——同步信号回收](#讨论——同步信号回收)）
  + 通过系统调用实现，或创建该屏障的进程结束/杀死后，自动销毁
  + 释放堵塞队列，并重置相关量

---
#### 时钟中断下的原子性保证
+ 关于屏障和条件变量的修改都只在内核态完成。通过大内核锁保证同一时刻只有一个进程进入内核态执行相关修改（包含单/双核架构），从而保证了并发处理屏障、条件变量等的正确性。
+ 对于时间中断，如上所述，同一时刻只有一个进程访问内核，包括通过时钟中断进入的进程。另外，在进程处于内核态时，将屏蔽时钟中断，因此修改过程不会被中断，可视作原子操作。
  
#### 细粒度内核锁/内核允许时间中断下的原子性
+ 需要在每次访问共享变量前后申请和释放自旋锁

---
<!--_class: lead-->
# Question 4:
### Show the structure for mailbox. 
### How do you protect concurrent 
### accessing for mailbox?

---
### Mailbox
+ 结构定义
```c
typedef enum {
    CLOSED,
    OPEN,
} mbox_status_t;
typedef struct mailbox{
    char name[20];
    char buf[MAX_MBOX_LENGTH];
    mbox_status_t status;
    int  index; //当前填充位置
    int  visitor;
    list_head full_queue;
    list_head empty_queue;
} mailbox_t;
```

---
+ 全局初始化：清空相关量
+ 开启
  + 遍历，如有已开启的对应信箱，则使用；否则再次遍历，开启空闲信箱并记录信箱名
  + 对信箱，增加访问者计数
  + 对进程，统计使用的信箱，以数组存储

+ 关闭（部分分析见[讨论——同步信号回收](#讨论——同步信号回收)）
  + 减少信箱访问者数量
  + 当访问数为0时，重置相关变量


---
+ 发送
  + 循环判断信箱接受信息后是否会超过其容量，如是，堵塞该发送进程并调度，同时记录阻塞次数；否则继续
  + 可接收信息时，将信息填入当前index指示位置，并相应修改index
  + 填入完毕后，释放因信箱为空堵塞的接收进程。

+ 接收
  + 循环判断信箱当前内容是否满足提取长度，如不满足，堵塞该发送进程并调度，同时记录阻塞次数；否则继续
  + 可提取信息时，从头部提取，并将剩余内容前移，相应修改index
  + 提取完毕后，释放因信箱为满堵塞的发送进程。
  
---
### 并发访问保护
+ 如前所述，由于实现内核锁，同一时刻只有一个进程会更改信箱内容，相当于相关修改操作均具有原子性（不可打断）
+ 当阻塞队列释放时，会将其中进程全部更改为就绪态。然而，可能当部分进程修改信箱内容后，其余进程不可再进行修改，因此进行循环判断，将不满足条件的进程再次阻塞。
+ 通过index变量，保证了不同发送进程填入信箱位置不重叠，也不覆盖未提取信息。从而保证了数据传递的正确性。

---
<!--_class: lead-->
# Question 5:
### How do you enable two CPU cores?

---
### 启动
+ bootblock时，从核将循环等待，接受到核间中断才可跳转到内核程序入口
+ 主核在完成全局初始化的相关操作，会先申请内核锁，再发送核间中断使从核进入内核入口。
+ 从核在进入后也会申请锁，等待主核完成第一次调度时释放锁，从核才进入内核态开始执行。
+ 从核开始执行时，大部分初始化工作都已由主核完成。由于从核具有自己的一套寄存器，因此需要设置中断入口，同时也需要将从核当前运行进程指针current_running_1指向当前进程。
---
### 内核锁的申请与释放
+ 对于上锁，无论是系统调用还是时钟中断，都应当在获得内核锁之后进行访问，因此应当放置在进入具体处理函数之前。同时需要应当在旧进程上下文保存之后，因此将申请内核锁放置在分发函数interrupt_helper之中。
+ 第一次上锁在main函数中完成，保证第一次调度的互斥性。
+ 对于释放锁，因为所有进程退出内核态都需要释放锁，为了利用第一次调度到用户进程时switchto，以及用户进程中断处理结束均返回到ret_from_exception的特点，放置在该函数中，且在恢复上下文前。
+ 对于调度到初始进程的情况，不会进入到ret_from_exception，在main函数的while循环中释放锁并再次申请。
  
---
### current_running的指定
+ 实现大内核锁之后，某一时刻至多只有一个进程访问内核态，因此可以复用current_running为current_running_0或current_running_1
+ 对于通过异常或中断进入内核态的进程，可以在分发函数中直接指定，但应当在申请到内核锁之后，以避免被其余进程修改。
+ 考虑到第一次调度时不经过interrupt_helper，在调度函数中再进行一次指定。
+ 在调度函数中，将会根据CPU的id更新curreng_running_0/1到新进程，在下一次指定时也就完成了current_running的更新

---
### 调度
+ 双核结构下，首先会遍历共有的就绪队列，寻找mask与该CPU的id相符的待调度新进程。
+ 如果未找到新进程，则需要调度到该CPU的初始进程，并在main函数中进行锁的释放和再次申请，从而防止一个CPU始终占据内核锁，使双核退化为单核。
+ 调度完成后，需要根据CPU进行相应指针的更新。
  
---
<!--_class: lead-->
# 讨论——同步信号回收

---
### 锁
+ 如前所述，一个锁至多由一个进程持有，而一个进程可以持有多把锁。从开销和实现复杂度考虑，在锁上标记占用者pid，杀死进程后，遍历锁数组进行释放。
### Barrier & Condition Variables
+ 这两个结构可以由多个进程访问，且每个使用的进程都直接使用id进行访问。由于此类结构实际上由父进程创建，（对屏障或条件变量不可知数量的）子进程访问。因此可以考虑由父进程在退出或杀死时进行销毁。

---
### Mailbox
+ 在信箱的使用中，并非由父进程创建，子进程使用。而是每个使用到该信箱的进程都会对其发起请求，并由信箱记录访问者数量。因此可以在每个使用者退出/杀死时减少信箱的使用者数量。当使用者为零时，进行信箱的关闭。

---
<!--_class: lead-->
# 讨论——进程回收

---
### 杀死子进程与释放同步信号
+ 如前所述，Barrier和Condition Variable的销毁应当由创建者（父进程）负责，通过系统调用实现，或在退出/销毁时自动完成。
+ 在销毁该屏障时，应当保证使用者均已结束。因此在杀死父进程时，也应当先行杀死子进程。
+ 考虑到以下情形，对于父进程创建的屏障，有3个子进程进行使用。当其中1个子进程被杀死时，仍处于堵塞队列，且该堵塞队列不再满足条件进行调度，并取消PCB块占用标记。此时，虽然资源可被释放，但PCB块不可使用，也即类似僵尸进程。当此类情况过多时，就陷入了PCB块不足的情况。
+ 对于上述情形，可以使用进程组的思想，通过杀死父进程来真正杀死僵尸子进程。

---
### 释放资源与取消PCB块占用标记
+ 首先，需要保证PCB块占用标记在就绪队列遍历到该值时进行取消。否则，可能该进程被杀死，但仍遗留在某个阻塞队列中。此时新创建的进程利用了该取消占用标记的PCB块，导致了新进程同时处于两个队列中，造成了错误。
+ 资源的释放应当在杀死该进程时完成。考虑上一部分中提到的屏障子进程被杀死情形。如果此时不进行资源的释放，屏障堵塞的进程因为不满足条件而不再调度，而父进程处于某个子进程的等待队列中，也无法再进行调度。这将导致即使父子进程都被杀死，仍然均未释放资源和取消PCB块占用。
+ 综上，在杀死进程时进行资源释放，在调度时取消PCB块占用。