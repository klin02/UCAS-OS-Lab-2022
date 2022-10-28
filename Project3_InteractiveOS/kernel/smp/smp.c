#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <printk.h>

//设置大锁，自旋锁
//0 UNLOCKED 1 LOCKED
spin_lock_t kernel_lock;

void smp_init()
{
    /* TODO: P3-TASK3 multicore*/
    kernel_lock.status = UNLOCKED; 
}

void wakeup_other_hart()
{
    /* TODO: P3-TASK3 multicore*/
    disable_sw_interrupt();
    send_ipi(NULL);
    clear_sw_interrupt();
    enable_sw_interrupt();
}

void lock_kernel()
{
    /* TODO: P3-TASK3 multicore*/
    while(atomic_swap_d(LOCKED,(ptr_t)&kernel_lock.status)==LOCKED)
    ;
    // printk("Lock is used!\n");
}

void unlock_kernel()
{
    /* TODO: P3-TASK3 multicore*/
    kernel_lock.status = UNLOCKED;
    // printk("Lock is release!\n");
}
