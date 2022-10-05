#include <common.h>
#include <asm.h>
#include <asm/unistd.h>
#include <os/loader.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/time.h>
#include <sys/syscall.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>

#define NUM_MAX_THREAD 10
extern void ret_from_exception();
pcb_t tcb[NUM_MAX_THREAD]; //全局变量，防止局部变量被丢弃
pid_t thread_ptr = 0;
void thread_create(ptr_t funcaddr,void *arg){ // void *就是函数地址
        int tid = *(int *)arg;
        tcb[thread_ptr].pid = current_running->pid; //与主线程一致
        tcb[thread_ptr].tid = tid;
        tcb[thread_ptr].wakeup_time = 0;
        tcb[thread_ptr].kernel_sp = allocKernelPage(1)+PAGE_SIZE;
        tcb[thread_ptr].user_sp = allocUserPage(1)+PAGE_SIZE;
        tcb[thread_ptr].status = TASK_READY;
        tcb[thread_ptr].lock_time = 0;
        //init reg on kernel stack
        regs_context_t *pt_regs = 
        (regs_context_t *)(tcb[thread_ptr].kernel_sp-sizeof(regs_context_t));
        for(int i=0;i<32;i++)
                pt_regs->regs[i]=0;
        //设置用户栈
        pt_regs->regs[2] = tcb[thread_ptr].user_sp;
        pt_regs->regs[4] = (reg_t)&tcb[thread_ptr];
        //设置跳转地址和传参
        pt_regs->sepc = funcaddr; // 返回到对应函数的入口
        pt_regs->regs[10] = arg; //a0 传参，注意，需要和sys_thread_create函数相统一，仍是地址
        //设置用户态
        pt_regs->sstatus = SR_SPIE & ~SR_SPP;
        pt_regs->sbadaddr = 0;
        pt_regs->scause =0;

        switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

        tcb[thread_ptr].kernel_sp = tcb[thread_ptr].kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t); 
        pt_switchto->regs[0] = (reg_t)&ret_from_exception;
        pt_switchto->regs[1] = tcb[thread_ptr].kernel_sp;

        enqueue(&ready_queue,&tcb[thread_ptr]);
        thread_ptr++;
}