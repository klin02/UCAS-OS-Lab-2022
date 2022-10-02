#include <syscall.h>
#include <stdint.h>

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

// int sys_mutex_init(int key)
// {
//     /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_init */
//     return 0;
// }

// void sys_mutex_acquire(int mutex_idx)
// {
//     /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_acquire */
// }

// void sys_mutex_release(int mutex_idx)
// {
//     /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_release */
// }

//多锁机制下，上述三个函数有所改动
void sys_mutex_init(void)
{
    invoke_syscall(SYSCALL_LOCK_INIT,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_mutex_acquire(void)
{
    invoke_syscall(SYSCALL_LOCK_ACQ,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_mutex_release(void)
{
    invoke_syscall(SYSCALL_LOCK_RELEASE,IGNORE,IGNORE,IGNORE,IGNORE,IGNORE);
}


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