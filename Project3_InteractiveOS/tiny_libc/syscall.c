#include <syscall.h>
#include <stdint.h>
#include <unistd.h>

static const long IGNORE = 0L;

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

void sys_yield(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_yield */
    invoke_syscall(SYSCALL_YIELD,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_move_cursor(int x, int y)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_move_cursor */
    invoke_syscall(SYSCALL_CURSOR,(long)x,(long)y,IGNORE,IGNORE,IGNORE);
}

void sys_write(char *buff)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_write */
    invoke_syscall(SYSCALL_WRITE,(long)buff,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_reflush(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_reflush */
    invoke_syscall(SYSCALL_REFLUSH,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

int sys_mutex_init(int key)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_init */
    return invoke_syscall(SYSCALL_LOCK_INIT,(long)key,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_mutex_acquire(int mutex_idx)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_acquire */
    invoke_syscall(SYSCALL_LOCK_ACQ,(long)mutex_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_mutex_release(int mutex_idx)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_release */
    invoke_syscall(SYSCALL_LOCK_RELEASE,(long)mutex_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

//多锁机制下，上述三个函数有所改动
// void sys_mutex_init(int key)
// {
//     invoke_syscall(SYSCALL_LOCK_INIT,(long)key,IGNORE,IGNORE,IGNORE,IGNORE);
// }

// void sys_mutex_acquire(void)
// {
//     invoke_syscall(SYSCALL_LOCK_ACQ,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
// }

// void sys_mutex_release(void)
// {
//     invoke_syscall(SYSCALL_LOCK_RELEASE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
// }


long sys_get_timebase(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_get_timebase */
    return invoke_syscall(SYSCALL_GET_TIMEBASE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

long sys_get_tick(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_get_tick */
    return invoke_syscall(SYSCALL_GET_TICK,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_sleep(uint32_t time)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_sleep */
    invoke_syscall(SYSCALL_SLEEP,(long)time,IGNORE,IGNORE,IGNORE,IGNORE);
}

//TASK5: create thread
// void sys_thread_create(void *func,void *arg)
// {
//     invoke_syscall(SYSCALL_THREAD_CREATE,(long)func,(long)arg,IGNORE,IGNORE,IGNORE);
// }

//Additional func: recycle thread
void sys_thread_create(void *func,void *arg,void *rc_func) //回收函数地址
{
    invoke_syscall(SYSCALL_THREAD_CREATE,(long)func,(long)arg,(long)rc_func,IGNORE,IGNORE);
}

void sys_thread_recycle(){
    invoke_syscall(SYSCALL_THREAD_RECYCLE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

// S-core
// pid_t  sys_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2)
// {
//     /* TODO: [p3-task1] call invoke_syscall to implement sys_exec for S_CORE */
// }    
// A/C-core
pid_t  sys_exec(char *name, int argc, char **argv)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exec */
    return invoke_syscall(SYSCALL_EXEC,(long)name,(long)argc,(long)argv,IGNORE,IGNORE);
}

void  sys_runmask(char *name, int argc, char **argv,int mask)
{
    invoke_syscall(SYSCALL_EXEC,(long)name,(long)argc,(long)argv,(long)mask,IGNORE);
}

void sys_setmask(pid_t pid, int mask)
{
    invoke_syscall(SYSCALL_SETMASK,(long)pid,(long)mask,IGNORE,IGNORE,IGNORE);
}

void sys_backspace(void)
{
    invoke_syscall(SYSCALL_BACKSPACE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_clear(void){
    invoke_syscall(SYSCALL_CLEAR,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}
void sys_exit(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exit */
    invoke_syscall(SYSCALL_EXIT,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

int  sys_kill(pid_t pid)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_kill */
    return invoke_syscall(SYSCALL_KILL,(long)pid,IGNORE,IGNORE,IGNORE,IGNORE);
}

int  sys_waitpid(pid_t pid)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_waitpid */
    return invoke_syscall(SYSCALL_WAITPID,(long)pid,IGNORE,IGNORE,IGNORE,IGNORE);
}


void sys_ps(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_ps */
    invoke_syscall(SYSCALL_SHOW_TASK,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

pid_t sys_getpid()
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_getpid */
    return invoke_syscall(SYSCALL_GETPID,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

int  sys_getchar(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_getchar */
    return invoke_syscall(SYSCALL_READCH,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}


int  sys_barrier_init(int key, int goal)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrier_init */
    return invoke_syscall(SYSCALL_BARR_INIT,(long)key,(long)goal,IGNORE,IGNORE,IGNORE);
}

void sys_barrier_wait(int bar_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_wait */
    invoke_syscall(SYSCALL_BARR_WAIT,(long)bar_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_barrier_destroy(int bar_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_destory */
    invoke_syscall(SYSCALL_BARR_DESTROY,(long)bar_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

int sys_condition_init(int key)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_init */
    return invoke_syscall(SYSCALL_COND_INIT,(long)key,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_condition_wait(int cond_idx, int mutex_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_wait */
    invoke_syscall(SYSCALL_COND_WAIT,(long)cond_idx,(long)mutex_idx,IGNORE,IGNORE,IGNORE);
}

void sys_condition_signal(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_signal */
    invoke_syscall(SYSCALL_COND_SIGNAL,(long)cond_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_condition_broadcast(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_broadcast */
    invoke_syscall(SYSCALL_COND_BROADCAST,(long)cond_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_condition_destroy(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_destroy */
    invoke_syscall(SYSCALL_COND_DESTROY,(long)cond_idx,IGNORE,IGNORE,IGNORE,IGNORE);
}

int sys_mbox_open(char * name)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_open */
    return invoke_syscall(SYSCALL_MBOX_OPEN,(long)name,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_mbox_close(int mbox_id)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_close */
    invoke_syscall(SYSCALL_MBOX_CLOSE,(long)mbox_id,IGNORE,IGNORE,IGNORE,IGNORE);
}

int sys_mbox_send(int mbox_idx, void *msg, int msg_length)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_send */
    return invoke_syscall(SYSCALL_MBOX_SEND,(long)mbox_idx,(long)msg,(long)msg_length,IGNORE,IGNORE);
}

int sys_mbox_recv(int mbox_idx, void *msg, int msg_length)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_recv */
    return invoke_syscall(SYSCALL_MBOX_RECV,(long)mbox_idx,(long)msg,(long)msg_length,IGNORE,IGNORE);
}
