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
#include <sys/syscall.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>
#include <os/thread.h>

// 注意该地址应当与bootblock同步改变
#define USER_INFO_ADDR 0x52400000

//规定测试任务启动顺序
#define TASK_LIST_LEN 9
char task_name_list [16][10] = {"print1","print2","fly","lock1","lock2","mylock","sleep","timer","mythread"};
// #define TASK_LIST_LEN 2
// char task_name_list [16][10] = {"print1","mythread"};
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

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
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
    memcpy(&tasknum,ptr,2);
    ptr += 2;
    for(int i=0;i<tasknum;i++){
        memcpy(&tasks[i],ptr,sizeof(task_info_t));
        ptr += sizeof(task_info_t);
    }
}


static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)//当前kernel_stack仍处于栈顶，应当将pcb的该变量修改至switchto头部，从而使得swtch中可以直接切换
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
    //pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = user_stack; //此处为用户栈
    pt_regs->regs[4] = (reg_t)pcb; //tp指向自身，保证恢复上下文不变 未恢复tp

    //do scheduler的switch将返回ret from exception 其中将会恢复上下文并sret，这里返回函数入口
    pt_regs->sepc = entry_point;
    //根据讲义，需要初始化SPP(0)和SPIE(1)，分别表示之前特权级和使能状态
    //宏定义见csr.h
    pt_regs->sstatus = SR_SPIE & ~SR_SPP;
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

static void init_pcb(void)
{
    /* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
    //初始化ready queue
    list_init(&ready_queue);
    //part2:初始化sleep queue
    list_init(&sleep_queue);
    int task_id;
    for(int i=0;i<TASK_LIST_LEN;i++){
        for(int j=0;j<tasknum;j++){
            if(strcmp(tasks[j].name,task_name_list[i])==0){
                task_id = j;
                break;
            }
        }
        pcb[process_id].pid=process_id;     //完成初始化后加1
        //初始化主线程tid为-1
        pcb[process_id].tid=-1;
        pcb[process_id].wakeup_time = 0;
        pcb[process_id].kernel_sp = allocKernelPage(1)+PAGE_SIZE;   //alloc返回的是栈底，需要先移动到栈顶再填数据
        pcb[process_id].user_sp = allocUserPage(1)+PAGE_SIZE;
        //注意task.entry只是镜像中偏移，实际计算需要通过taskid
        ptr_t task_entrypoint = TASK_MEM_BASE + task_id*TASK_SIZE;
        init_pcb_stack(pcb[process_id].kernel_sp,pcb[process_id].user_sp,task_entrypoint,&pcb[process_id]);
        pcb[process_id].status = TASK_READY;
        //为多锁准备
        pcb[process_id].lock_time = 0;
        //cursor wakuptime暂时不初始化，list在入队列时初始化。
        enqueue(&ready_queue,&pcb[process_id]);
        process_id++;
    }
    /* TODO: [p2-task1] remember to initialize 'current_running' */
    current_running = &pid0_pcb;
}

static void init_syscall(void)
{
    // TODO: [p2-task3] initialize system call table.
    // 与跳转表实现略有不同，并非固定地址，相应数组已定义在syscall.c中
    // 对应函数分别位于sched.c screen.c time.c lock.c
    syscall[SYSCALL_SLEEP]           = (long(*)())do_sleep;
    syscall[SYSCALL_YIELD]          = (long(*)())do_scheduler;
    syscall[SYSCALL_WRITE]          = (long(*)())screen_write;
    syscall[SYSCALL_CURSOR]         = (long(*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH]        = (long(*)())screen_reflush;
    syscall[SYSCALL_GET_TIMEBASE]   = (long(*)())get_time_base;
    syscall[SYSCALL_GET_TICK]       = (long(*)())get_ticks;
    syscall[SYSCALL_LOCK_INIT]      = (long(*)())do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQ]       = (long(*)())do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE]   = (long(*)())do_mutex_lock_release;
    syscall[SYSCALL_THREAD_CREATE]  = (long(*)())thread_create;
    syscall[SYSCALL_THREAD_RECYCLE] = (long(*)())thread_recycle;
}

int main(void)
{
    // 新增：初始化可回收内存分配机制
    init_mm();
    
    // Init jump table provided by kernel and bios(ΦωΦ)
    init_jmptab();

    // Init task information (〃'▽'〃)
    init_task_info();
    
    //加载全部程序
    for(int i=0;i<tasknum;i++)
        load_task_img(i);
    
    // Init Process Control Blocks |•'-'•) ✧
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n");

    

    // Read CPU frequency (｡•ᴗ-)_
    time_base = bios_read_fdt(TIMEBASE);
    printk("timebase %ld\n",time_base);

    // Init lock mechanism o(´^｀)o
    init_locks();
    printk("> [INIT] Lock mechanism initialization succeeded.\n");

    // Init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    // Init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    // Init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");

    // TODO: [p2-task4] Setup timer interrupt and enable all interrupt globally
    // NOTE: The function of sstatus.sie is different from sie's

    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        // If you do non-preemptive scheduling, it's used to surrender control
        // do_scheduler();

        // If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
        enable_preempt();
        //在此设置第一个定时器中断，以激发第一次调度
        set_timer(get_ticks()+time_base/100);
        do_scheduler();

        asm volatile("wfi");
    }

    return 0;
}
