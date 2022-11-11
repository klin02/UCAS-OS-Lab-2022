/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

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
#include <os/smp.h>
#include <sys/syscall.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>
#include <os/thread.h>
#include <pgtable.h>

// 注意该地址应当与bootblock同步改变
#define USER_INFO_ADDR 0xffffffc052400000

#define ARGV_OFFSET 64
#define ARG_SIZE    128

char * shellptr = "shell";
char * idleptr = "idle";
//以下均已在sched.c/sched.h声明
// /* current running task PCB */
// extern pcb_t * volatile current_running;
// extern pid_t process_id;

// extern pcb_t pcb[NUM_MAX_TASK];
// extern pcb_t pid0_pcb;
extern void ret_from_exception();

// Task info array
task_info_t tasks[TASK_MAXNUM];
short tasknum;
//新增入队列和出队列函数，方便队列维护
//定义在sched.h中，实现在sched.c中

static void cancel_tmp_map(){
    PTE *early_pgdir = pa2kva(PGDIR_PA);
    //0x50000000~0x51000000 vpn2均为1
    clear_pgdir(pa2kva(get_pa(early_pgdir[1])));
    early_pgdir[1] = 0;
}

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
    jmptab[SD_WRITE]        = (long (*)())sd_write;
    jmptab[QEMU_LOGGING]    = (long (*)())qemu_logging;
    jmptab[SET_TIMER]       = (long (*)())set_timer;
    jmptab[READ_FDT]        = (long (*)())read_fdt;
    jmptab[MOVE_CURSOR]     = (long (*)())screen_move_cursor;
    jmptab[PRINT]           = (long (*)())printk;
    jmptab[YIELD]           = (long (*)())do_scheduler;
    jmptab[MUTEX_INIT]      = (long (*)())do_mutex_lock_init;
    jmptab[MUTEX_ACQ]       = (long (*)())do_mutex_lock_acquire;
    jmptab[MUTEX_RELEASE]   = (long (*)())do_mutex_lock_release;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    // 根据用户信息扇区获取（已加载到内存），只需加载选定程序即可，在load中实现
    unsigned char * ptr = (unsigned long *)USER_INFO_ADDR;
    ptr += 8;
    //tasknum is define in task.h
    memcpy((unsigned char *)&tasknum,ptr,2);
    ptr += 2;
    for(int i=0;i<tasknum;i++){
        memcpy((unsigned char *)&tasks[i],ptr,sizeof(task_info_t));
        ptr += sizeof(task_info_t);
    }
}


void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb,int argc, char **argv_base)//当前kernel_stack仍处于栈顶，应当将pcb的该变量修改至switchto头部，从而使得swtch中可以直接切换
{
     /* TODO: [p2-task3] initialization of registers on kernel stack
      * HINT: sp, ra, sepc, sstatus
      * NOTE: To run the task in user mode, you should set corresponding bits
      *     of sstatus(SPP, SPIE, etc.).
      */
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    //task3： 需要将其余regs清零，设置sp ra。 设置sepc和sstatus符合用户态
    for(int i=0;i<32;i++)
        pt_regs->regs[i]=0;
    //详细布局见regs.h
    //pt_regs->regs[1] = (reg_t)rc_addr;
    pt_regs->regs[2] = user_stack; //此处为用户栈
    pt_regs->regs[4] = (reg_t)pcb; //tp指向自身，保证恢复上下文不变 未恢复tp
    pt_regs->regs[10] = (reg_t)argc; //a0
    pt_regs->regs[11] = (reg_t)argv_base; //a1
    //do scheduler的switch将返回ret from exception 其中将会恢复上下文并sret，这里返回函数入口
    pt_regs->sepc = entry_point;
    //根据讲义，需要初始化SPP(0)和SPIE(1)，分别表示之前特权级和使能状态
    //宏定义见csr.h
    pt_regs->sstatus = ( SR_SPIE & ~SR_SPP ) | SR_SUM;
    pt_regs->sbadaddr = 0;
    pt_regs->scause =0;

    /* TODO: [p2-task1] set sp to simulate just returning from switch_to
     * NOTE: you should prepare a stack, and push some values to
     * simulate a callee-saved context.
     */
    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));
    //swtch存取寄存器的基址
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pt_switchto->regs[0] = (reg_t)&ret_from_exception; //ra 函数需要取地址
    pt_switchto->regs[1] = pcb->kernel_sp; //sp
}

pid_t init_pcb(char *name, int argc, char *argv[])
{
    /* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
    int isshell=0;
    if(strcmp(name,shellptr)==0)
        isshell=1;
    int mask;
    int ppid;
    //shell使用默认值，其余继承父值
    if(isshell==1){
    //初始化ready queue
    list_init(&ready_queue);
    //part2:初始化sleep queue
    list_init(&sleep_queue);     
    mask = 3;   
    ppid = 0;
    }
    else{
        mask = current_running -> mask;
        ppid = current_running -> pid;
    }
    
    int task_id=-1;
    for(int j=0;j<tasknum;j++){
        if(strcmp(tasks[j].name,name)==0){
            task_id = j;
            break;
        }
    }

    if(task_id == -1)
        return 0; //不存在该任务，直接返回0（非法）
    int hitid=-1;
    for(int i=0;i<NUM_MAX_TASK;i++)
        if(pcb_flag[i]==0)
        {
            hitid=i;
            pcb_flag[i]=1;
            break;
        }
        
    assert(hitid>=0);
    int pid=hitid+1;
    pcb[hitid].pid=pid;     //完成初始化后加1
    pcb[hitid].ppid = ppid;
    //初始化主线程tid为-1
    pcb[hitid].tid=-1;
    //初始化页数组
    pcb[hitid].pg_num = 0;
    for(int i=0;i<128;i++)
        pcb[hitid].pg_addr[i] = 0;
    pcb[hitid].wakeup_time = 0;
    pcb[hitid].pgdir = allocPage(1,&pcb[hitid]);
    clear_pgdir(pcb[hitid].pgdir);
    share_pgtable(pcb[hitid].pgdir,pa2kva(PGDIR_PA));
    //注意：内核栈需要内核地址空间中占据页表。已在boot.c中完成映射
    pcb[hitid].kernel_stack_base = allocPage(1,&pcb[hitid]);
    pcb[hitid].kernel_sp = pcb[hitid].kernel_stack_base + PAGE_SIZE - 128;  
    //用户栈使用用户地址空间，相互独立
    //注意不能正好处于下一页的页头
    pcb[hitid].user_stack_base = USER_STACK_ADDR;
    pcb[hitid].user_sp = pcb[hitid].user_stack_base + PAGE_SIZE - 128;
    //注意：当前usr_sp为栈顶，也即页表末尾，传入的应当为页表起始地址
    ptr_t usr_sp_pg_kva = alloc_page_helper(USER_STACK_ADDR, pcb[hitid].pgdir,&pcb[hitid]);

    load_task_img(task_id,pcb[hitid].pgdir,&pcb[hitid]);
    list_init(&pcb[hitid].wait_queue);
    for(int i=0;i<MBOX_NUM;i++){
        pcb[hitid].mbox_arr[i]=0;
    }
    pcb[hitid].mbox_cnt =0;
    pcb[hitid].mask = mask;
    char **argv_base_kva;
    char **argv_base_uva;
    if(isshell==0)
    {
        //参数排布——虚存模式
        //尾缀kva为内核虚地址，用于拷贝
        //uva为用户虚地址，用于记录
        argv_base_kva = (char **)(usr_sp_pg_kva + PAGE_SIZE - 128 - ARGV_OFFSET);
        argv_base_uva = (char **)(pcb[hitid].user_sp - ARGV_OFFSET);
        char *strptr_kva = (char *)argv_base_kva;
        char *strptr_uva = (char *)argv_base_uva;
        char **argvptr= argv_base_kva;
        for(int i=0;i<argc;i++)
        {
            strptr_kva -= strlen(argv[i])+1;
            strptr_uva -= strlen(argv[i])+1;
            strcpy(strptr_kva,argv[i]);
            *argvptr = strptr_uva;
            argvptr ++; //注意类型，这里每加1，内存位置偏移sizeof(char *)
        } 
        pcb[hitid].user_sp -= ARG_SIZE; //进行偏移，注意对齐
    }
    else{
        argv_base_uva = 0;
    }

    //注意task.entry只是镜像中偏移，实际计算需要通过taskid
    ptr_t task_entrypoint = tasks[task_id].vaddr;
    init_pcb_stack(pcb[hitid].kernel_sp,pcb[hitid].user_sp,task_entrypoint,&pcb[hitid],argc,argv_base_uva);
    pcb[hitid].status = TASK_READY;
    //为多锁准备
    pcb[hitid].lock_time = 0;
    enqueue(&ready_queue,&pcb[hitid]);
    return pid;
}

void init_idle_pcb(){
    int idle_id=-1;
    for(int i=0;i<tasknum;i++){
        if(strcmp(tasks[i].name,idleptr)==0){
            idle_id = i;
            break;
        }
    }
    for(int i=0;i<2;i++)
    {
        idle_pcb[i].pid=0;
        idle_pcb[i].pg_num = 0;
        for(int j=0;j<128;j++)
            idle_pcb[i].pg_addr[j] = 0;
        idle_pcb[i].pgdir = allocPage(1,&idle_pcb[i]);
        clear_pgdir(idle_pcb[i].pgdir);
        share_pgtable(idle_pcb[i].pgdir,pa2kva(PGDIR_PA));
        //注意：内核栈需要内核地址空间中占据页表。已在boot.c中完成映射
        idle_pcb[i].kernel_stack_base = allocPage(1,&idle_pcb[i]);
        idle_pcb[i].kernel_sp = idle_pcb[i].kernel_stack_base + PAGE_SIZE - 128;  
        //用户栈使用用户地址空间，相互独立
        //注意不能正好处于下一页的页头
        idle_pcb[i].user_stack_base = USER_STACK_ADDR;
        idle_pcb[i].user_sp = idle_pcb[i].user_stack_base + PAGE_SIZE - 128;
        //注意：当前usr_sp为栈顶，也即页表末尾，传入的应当为页表起始地址
        alloc_page_helper(USER_STACK_ADDR, idle_pcb[i].pgdir,&idle_pcb[i]);

        load_task_img(idle_id,idle_pcb[i].pgdir,&idle_pcb[i]);
        ptr_t task_entrypoint = tasks[idle_id].vaddr;
        init_pcb_stack(idle_pcb[i].kernel_sp,idle_pcb[i].user_sp,task_entrypoint,&idle_pcb[i],0,NULL);

    }

}

static void init_syscall(void)
{
    // TODO: [p2-task3] initialize system call table.
    // 与跳转表实现略有不同，并非固定地址，相应数组已定义在syscall.c中
    // 对应函数分别位于sched.c screen.c time.c lock.c
    syscall[SYSCALL_SLEEP]          = (long(*)())do_sleep;
    syscall[SYSCALL_YIELD]          = (long(*)())do_scheduler;
    syscall[SYSCALL_WRITE]          = (long(*)())screen_write;
    syscall[SYSCALL_READCH]         = (long(*)())port_read_ch;
    syscall[SYSCALL_CURSOR]         = (long(*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH]        = (long(*)())screen_reflush;
    syscall[SYSCALL_CLEAR]          = (long(*)())screen_clear;
    syscall[SYSCALL_BACKSPACE]      = (long(*)())screen_backspace;
    syscall[SYSCALL_GET_TIMEBASE]   = (long(*)())get_time_base;
    syscall[SYSCALL_GET_TICK]       = (long(*)())get_ticks;
    syscall[SYSCALL_LOCK_INIT]      = (long(*)())do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQ]       = (long(*)())do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE]   = (long(*)())do_mutex_lock_release;
    syscall[SYSCALL_THREAD_CREATE]  = (long(*)())thread_create;
    syscall[SYSCALL_THREAD_RECYCLE] = (long(*)())thread_recycle;
    syscall[SYSCALL_SHOW_TASK]      = (long(*)())do_process_show;
    syscall[SYSCALL_EXEC]           = (long(*)())do_exec;
    syscall[SYSCALL_EXIT]           = (long(*)())do_exit;
    syscall[SYSCALL_KILL]           = (long(*)())do_kill;
    syscall[SYSCALL_WAITPID]        = (long(*)())do_waitpid;
    syscall[SYSCALL_GETPID]         = (long(*)())do_getpid;
    syscall[SYSCALL_BARR_INIT]      = (long(*)())do_barrier_init;
    syscall[SYSCALL_BARR_WAIT]      = (long(*)())do_barrier_wait;
    syscall[SYSCALL_BARR_DESTROY]   = (long(*)())do_barrier_destroy;
    syscall[SYSCALL_COND_INIT]      = (long(*)())do_condition_init;
    syscall[SYSCALL_COND_WAIT]      = (long(*)())do_condition_wait;
    syscall[SYSCALL_COND_SIGNAL]    = (long(*)())do_condition_wait;
    syscall[SYSCALL_COND_BROADCAST] = (long(*)())do_condition_broadcast;
    syscall[SYSCALL_COND_DESTROY]   = (long(*)())do_condition_destroy;
    syscall[SYSCALL_MBOX_OPEN]      = (long(*)())do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE]     = (long(*)())do_mbox_close;
    syscall[SYSCALL_MBOX_SEND]      = (long(*)())do_mbox_send;
    syscall[SYSCALL_MBOX_RECV]      = (long(*)())do_mbox_recv;
    syscall[SYSCALL_RUNMASK]        = (long(*)())do_runmask;
    syscall[SYSCALL_SETMASK]        = (long(*)())do_setmask;
}

int main(void)
{
    cancel_tmp_map();
    if(get_current_cpu_id() == 0)
    {
    current_running_0 = &pid0_pcb;
    current_running_1 = &pid1_pcb;
    current_running = current_running_0;

        // 新增：初始化可回收内存分配机制
    init_mm();
    
    // Init jump table provided by kernel and bios(ΦωΦ)
    init_jmptab();
    printk("Enter main\n");
    // Init task information (〃'▽'〃)
    init_task_info();
    
    // Init Process Control Blocks |•'-'•) ✧
    init_pcb(shellptr,0,NULL); //只初始化shell进程
    init_idle_pcb();
    printk("> [INIT] PCB initialization succeeded.\n");

    // Read CPU frequency (｡•ᴗ-)_
    time_base = bios_read_fdt(TIMEBASE);
    printk("timebase %ld\n",time_base);

    // Init lock mechanism o(´^｀)o
    init_locks();
    printk("> [INIT] Lock mechanism initialization succeeded.\n");

    // 初始化同步屏障
    init_barriers();
    printk("> [INIT] Barriers initialization succeeded.\n");

    // Init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    // Init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    // Init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");

    smp_init();
    lock_kernel(); //只能有一个CPU访问内核空间,调度时再释放。
    wakeup_other_hart();
    // unlock_kernel();
    // while(1);
    //enable_preempt();
    }
    else{
        lock_kernel();
        // unlock_kernel();
        // while(1);
        current_running = current_running_1;
        setup_exception();
    }

    // TODO: [p2-task4] Setup timer interrupt and enable all interrupt globally
    // NOTE: The function of sstatus.sie is different from sie's
    
    //考虑到可能下一次进程需要多次寻找，将从pcb0/1到其余进程的时钟设置放置在do_scheduler中
    while (1)
    {
        // If you do non-preemptive scheduling, it's used to surrender control
        // do_scheduler();

        // If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
        set_timer(get_ticks()+TIMER_INTERVAL);
        do_scheduler();
        //由于未经过ret_from_exception，需要允许其他核抢到锁
        // unlock_kernel();
        // while(1);
        // lock_kernel();
    }

    return 0;
}
