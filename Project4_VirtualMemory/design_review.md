---
marp: true
theme: gaia 
paginate: true

header: Prj4_VirtualMemory &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 游昆霖 2020K8009926006
---
<!--_class: lead-->
# Project_4 &ensp;VirtualMemory
## —— Design Review
### 游昆霖 2020K8009926006

---
<!--_class: lead-->
# Question 1: 
### Show the data structure of your 
### page table and page table entry.
### Where do you place the page 
### table in physical memory?
### How large is your page table?

---
### 页表及页表项结构
+ 内核使用二级页表，用户使用三级页表
+ 用户L2级页表通过PCB块中的pgdir索引，L1和L0级页表通过上级页表的页表项索引。
+ 页表项位置 (PTE*)pgdir+vpn 页表项内容 pgdir[vpn]
+ 页表项结构：对应次级的物理地址 | 标志位

---
### 页表在真实内存中的分配
+ 内核页表物理地址由宏定义PGDIR_PA确定
+ 用户的页表存储于内核空间通过allocPage分配的页中，L2级页表的内核虚地址由PCB块的pgdir索引，其余低级页表的物理地址存于上级页表项中。
+ 页表大小为4K，其中含512个页表项。

---
<!--_class: lead-->
# Question 2:
### How do you set up user process page table
### and load program into VM from SD?

---
### 用户进程页表设置
+ 步骤流程：
  + allocPage 分配用户L2级页表
  + clear_pgdir 清空该页表
  + share_pgtable 将内核页表复制到用户L2页表中
  + alloc_page_helper 在用户虚拟地址空间为用户栈指针分配页
  + load_task_img 将程序的代码段和数据段载入用户的虚拟地址空间
    + 详见下一部分

---
+ 函数详解
  + clear_pgdir：将页表中的各个页表项清零
  + share_pgtable：整个页表的复制
  + alloc_page_helper：
    + 根据VPN2~VPN0逐级索引。
    + 如果页表项V标志位为低，则为下一级页表分配物理页，并更新该页表项，根据分配的页表内核虚地址继续索引；
    + 否则，提取页表项中物理地址并转为下一级页表的内核虚地址继续索引
    + 索引至L0级页表页表项，V为空时进行页分配，该物理页即为用户虚地址对应的页

---
### 用户程序的载入
+ 由于此时以4K大小页为内存分配单位，用户程序在真实内存中可能无法连续放置，因此只能每次8个扇区载入，在考虑对齐和填充。
+ 具体步骤：
  1. 计算镜像中程序的起始扇区和结束扇区（根据filesz）
  2. 根据虚地址进行页申请，并记录分配页的内核虚地址
  3. 将8个扇区载入分配页的物理地址，减少剩余扇区数，改变虚地址、起始扇区号
  4. 循环执行前两步，直至镜像中用户程序filesz的内容均被载入。
  5. 跨页拷贝，使得用户程序入口页对齐
  6. 根据memsz计算总页数，如大于当前已分配页，额外分配空页。
  
---
<!--_class: lead-->
# Question 3:
### How do you handle page fault?
### pls. provides workflow or pseudo code 

---
### 缺页错误的处理
+ 错误类型
  + 指令缺页错误：正确实现后不会触发
  + load操作页缺失：执行访问操作时，V为0或A为0
  + store操作页确实：执行写入操作时，V为0或A/D位为0
+ 处理流程
  1. 首先检查V位。若V为1，表示该页存在，根据错误类型相应置位即可。若V为0，执行缺页处理。
  2. 根据pid和页对齐的虚地址在swap区查找。如果找到，分配一个物理页，将swap页对应的页表项更新。否则继续。
  3. 在用户进程的页表中为缺页虚地址进行页分配。
   
---
<!--_class: lead-->
# Question 4:
### What is your replacement policy

---
### FIFO的实现
+ 使用数组模拟实现循环队列。
+ FIFO中使用页号索引可换出的页。
+ 为保证性能，仅允许存储用户程序内容（栈、代码、数据及访问部分）的物理页被换出，存储页表的物理页不可换出。
+ 入队情形：通过alloc_page_help分配具体对应虚地址的页，将会入队；从swap区换入的页将会入队。
+ 出队情形：当allocPage可分配物理页都被占用时，进行出队，该出队后的物理页将被重新分配。

---
### 测试
+ 设置可分配页为256/512，swap区分配页为4096
+ 测试程序将会进行两轮循环
  + 第一轮循环将依次写入4K页并读出，测试物理页不足时换出到swap区的正确性。
  + 第二轮循环将会将之前写入的4K页进行读取，测试swap区换入的正确性。
  
---
<!--_class: lead-->
# Question 5:
### What objects should be created to setup a thread?
### How to start/stop your thread?
### How to schedule a thread? 

---
### 线程管理结构
+ 仿照xv6，使用相同的数据结构pcb_t对线程进行管理，通过初始化流程进行区别。
+ 新增tid对同一个进程的多个线程进行区分，其中tid为0表示主线程，其余情况表示第i个创建的子进程
+ 设置thread_num以记录该进程的线程数

---
### 线程的创建与启动
+ 在线程的创建中，该线程的pid、ppid、pgdir等都会继承主线程的值。从而保证能够共享同一进程的虚拟地址空间。
+ 线程将会申请自己的内核栈和用户栈以便独自调度。内核栈将根据allocPage分配的空间确定，用户栈采用线性映射的方案，并映射到页表中 pcb[hitid].user_stack_base = USER_STACK_ADDR + current_running->thread_num * PAGE_SIZE;）
+ 线程有独自的信箱访问数组，以便在线程回收时进行信箱的关闭。
+ 在线程的内核栈设置中，sepc可设置为线程的入口，即传入的函数入口。ra可设置为线程回收的函数，进行线程结束后的处理。

---
### 线程的结束和回收
+ 如上所述，可以通过设置ra的方式指定线程的回收处理函数。该函数可以通过系统调用标记线程结束标志，并通知主线程进行回收。
+ pthread_join函数可以将主线程堵塞在子线程的等待队列中，子线程返回时，将结束标志置高，并释放主线程，有主线程完成相应回收工作。
  
---
### 线程的调度
+ 根据上述的初始化方案，线程的调度将与进程类似。
+ 特别的，在线程间切换的时候，由于共享同一个页表和同一个进程的地址空间，无需进行页表的切换，调度更加高效。