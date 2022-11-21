#include <os/pthread.h>
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

extern void ret_from_exception();
void do_pthread_create(pthread_t *thread,void (*start_routine)(void*),void *arg)
{
        //获取未使用的pcb块
        int hitid = -1;
        for(int i=0;i<NUM_MAX_TASK;i++)
                if(pcb_flag[i]==0)
                {
                        hitid=i;
                        pcb_flag[i]=1;
                        break;
                }

        assert(hitid >= 0);
        
        pcb[hitid].pid = current_running->pid;
        pcb[hitid].ppid = current_running->ppid;
        pcb[hitid].mask = current_running->mask;
        pcb[hitid].tid = ++current_running->thread_num;
        pcb[hitid].wakeup_time = 0;
        
        pcb[hitid].pgdir = current_running->pgdir;

        int kstack_id = allocPage(1);
        pcb[hitid].kernel_stack_base = ava_page[kstack_id].addr;
        ava_page[kstack_id].pid = current_running->pid;
        pcb[hitid].kernel_sp = pcb[hitid].kernel_stack_base + PAGE_SIZE - 128;
        
        //线性映射，在原进程用户栈周围页
        pcb[hitid].user_stack_base = USER_STACK_ADDR + current_running->thread_num * PAGE_SIZE;
        pcb[hitid].user_sp = pcb[hitid].user_stack_base + PAGE_SIZE - 128;
        ptr_t usr_sp_pg_kva = alloc_page_helper(USER_STACK_ADDR, pcb[hitid].pgdir,&pcb[hitid]);

        list_init(&pcb[hitid].wait_queue);
        for(int i=0;i<MBOX_NUM;i++){
                pcb[hitid].mbox_arr[i]=0;
        }
        pcb[hitid].mbox_cnt =0;

        ptr_t td_entrypoint = (ptr_t)start_routine;
        init_pcb_stack(pcb[hitid].kernel_sp,pcb[hitid].user_sp,td_entrypoint,&pcb[hitid],(int)arg,NULL);
        pcb[hitid].status = TASK_READY;
        enqueue(&ready_queue,&pcb[hitid]);
        *thread = pcb[hitid].tid;
}

int do_pthread_join(pthread_t thread){
        int find = 0;
        int child;
        for(int i=0;i<NUM_MAX_TASK;i++)
                if(pcb_flag[i] == 1 && pcb[i].pid == current_running->pid && pcb[i].tid == thread){
                        find =1;
                        child = i;
                }
        if(find == 1){
                do_block(&(current_running->list),&(pcb[child].wait_queue));
                do_scheduler();
                return 1;
        }
        else
                return 0;
}