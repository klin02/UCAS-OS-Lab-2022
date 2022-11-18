// #include <common.h>
// #include <asm.h>
// #include <asm/unistd.h>
// #include <os/loader.h>
// #include <os/irq.h>
// #include <os/sched.h>
// #include <os/lock.h>
// #include <os/kernel.h>
// #include <os/task.h>
// #include <os/string.h>
// #include <os/mm.h>
// #include <os/time.h>
// #include <sys/syscall.h>
// #include <screen.h>
// #include <printk.h>
// #include <assert.h>
// #include <type.h>
// #include <csr.h>

// #define NUM_MAX_THREAD 10
// extern void ret_from_exception();
// // pcb_t tcb[NUM_MAX_THREAD]; //全局变量，防止局部变量被丢弃
// // pid_t thread_ptr = 0;
// // void thread_create(ptr_t funcaddr,void *arg){ // void *就是函数地址
// //         int tid = *(int *)arg;
// //         tcb[thread_ptr].pid = current_running->pid; //与主线程一致
// //         tcb[thread_ptr].tid = tid;
// //         tcb[thread_ptr].wakeup_time = 0;
// //         tcb[thread_ptr].kernel_sp = allocKernelPage(1)+PAGE_SIZE;
// //         tcb[thread_ptr].user_sp = allocUserPage(1)+PAGE_SIZE;
// //         tcb[thread_ptr].status = TASK_READY;
// //         tcb[thread_ptr].lock_time = 0;
// //         //init reg on kernel stack
// //         regs_context_t *pt_regs = 
// //         (regs_context_t *)(tcb[thread_ptr].kernel_sp-sizeof(regs_context_t));
// //         for(int i=0;i<32;i++)
// //                 pt_regs->regs[i]=0;
// //         //设置用户栈
// //         pt_regs->regs[2] = tcb[thread_ptr].user_sp;
// //         pt_regs->regs[4] = (reg_t)&tcb[thread_ptr];
// //         //设置跳转地址和传参
// //         pt_regs->sepc = funcaddr; // 返回到对应函数的入口
// //         pt_regs->regs[10] = arg; //a0 传参，注意，需要和sys_thread_create函数相统一，仍是地址
// //         //设置用户态
// //         pt_regs->sstatus = SR_SPIE & ~SR_SPP;
// //         pt_regs->sbadaddr = 0;
// //         pt_regs->scause =0;

// //         switchto_context_t *pt_switchto =
// //         (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

// //         tcb[thread_ptr].kernel_sp = tcb[thread_ptr].kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t); 
// //         pt_switchto->regs[0] = (reg_t)&ret_from_exception;
// //         pt_switchto->regs[1] = tcb[thread_ptr].kernel_sp;

// //         enqueue(&ready_queue,&tcb[thread_ptr]);
// //         thread_ptr++;
// // }

// pcb_t tcb[NUM_MAX_THREAD]; //全局变量，防止局部变量被丢弃
// int tcb_flag[NUM_MAX_THREAD]={0}; //用于回收，标记tcb是否被占用
// void thread_create(ptr_t funcaddr,void *arg,ptr_t rc_funcaddr){ // void *就是函数地址
//         int tid = *(int *)arg;
//         int thread_ptr; //对应标号
//         int find=0;
//         for(int i=0;i<NUM_MAX_THREAD;i++){
//                 if(tcb_flag[i]==0){
//                         tcb_flag[i]=1;
//                         find=1;
//                         thread_ptr = i;
//                         break;
//                 }
//         }
//         if(find==0){
//                 assert(0);
//         }
        
//         tcb[thread_ptr].pid = current_running->pid; //与主线程一致
//         tcb[thread_ptr].tid = tid;
//         tcb[thread_ptr].tcb_num = thread_ptr;
//         tcb[thread_ptr].wakeup_time = 0;
//         tcb[thread_ptr].kernel_sp = ava_page[allocPage(1)].addr+PAGE_SIZE;
//         tcb[thread_ptr].user_sp = ava_page[allocPage(1)].addr+PAGE_SIZE;
//         tcb[thread_ptr].status = TASK_READY;
//         tcb[thread_ptr].lock_time = 0;
//         //init reg on kernel stack
//         regs_context_t *pt_regs = 
//         (regs_context_t *)(tcb[thread_ptr].kernel_sp-sizeof(regs_context_t));
//         for(int i=0;i<32;i++)
//                 pt_regs->regs[i]=0;
//         //设置用户栈
//         pt_regs->regs[2] =  (reg_t)tcb[thread_ptr].user_sp;
//         pt_regs->regs[4] = (reg_t)&tcb[thread_ptr];
//         //设置跳转地址和传参
//         pt_regs->sepc = funcaddr; // 返回到对应函数的入口
//         pt_regs->regs[10] = (reg_t)arg; //a0 传参，注意，需要和sys_thread_create函数相统一，仍是地址
//         //设置ra，使线程结束时返回回收函数
//         pt_regs->regs[1] =  (reg_t)rc_funcaddr;
//         //设置用户态
//         pt_regs->sstatus = SR_SPIE & ~SR_SPP;
//         pt_regs->sbadaddr = 0;
//         pt_regs->scause =0;

//         switchto_context_t *pt_switchto =
//         (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

//         tcb[thread_ptr].kernel_sp = tcb[thread_ptr].kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t); 
//         pt_switchto->regs[0] = (reg_t)&ret_from_exception;
//         pt_switchto->regs[1] = tcb[thread_ptr].kernel_sp;

//         enqueue(&ready_queue,&tcb[thread_ptr]);
// }

// void thread_recycle(){
//         //无需传参，主要参数可利用current_running传递
//         //功能：对应用户栈和内核栈取消占用标记，tcb块取消占用标记，调度
//         //回收内存
//         // int Kernel_page_num = (current_running->kernel_sp - FREEMEM_KERNEL) / PAGE_SIZE;
//         // int User_page_num = (current_running->user_sp - FREEMEM_USER) / PAGE_SIZE;
//         // freeKernelPage(Kernel_page_num);
//         // freeUserPage(User_page_num);
//         //回收tcb块
//         tcb_flag[current_running->tcb_num] = 0;
//         //更新current状态即可复用do_scheduler
//         current_running->status = TASK_EXITED;
//         do_scheduler();
// }