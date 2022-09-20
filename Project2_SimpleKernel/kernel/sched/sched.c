#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs

    printl("begin schedule\n");
    // TODO: [p2-task1] Modify the current_running pointer.
    //将cur加入ready queue。同时从中拿出next
    pcb_t * next_running;
    next_running = dequeue(&ready_queue);
    pcb_t * last_running;
    last_running = current_running;

    if(current_running->pid != 0){//task1中只需考虑pcb0不回收，后续任务需要考虑状态
        current_running->status = TASK_READY;
        enqueue(&ready_queue,current_running);
    }

    next_running->status = TASK_RUNNING;
    current_running = next_running;
    printl("begin swtch\n");
    printl("pid %d %d\n",last_running->pid,current_running->pid);
    printl("knsp %ld %ld\n",last_running->kernel_sp,current_running->kernel_sp);
    // TODO: [p2-task1] switch_to current_running
    switch_to(last_running,current_running);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: [p2-task2] block the pcb task into the block queue
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
}

//(type *)0强制转化为地址为0的type类型指针。下述宏定义获得了member成员相对于type指针入口的偏移
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//第一行将进行类型检查，确定ptr与member类型是否一致。第二行则是根据成员变量减去偏移值得到了容器值。
#define list_entry(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })


void enqueue(list_head* queue,pcb_t* pnode){
    list_node_t *lnode = &(pnode->list);
    list_add_tail(queue,lnode);
}

pcb_t * dequeue(list_head* queue){
    //判断非空时才可出队
    if(queue->prev == NULL)
        return NULL;
    
    pcb_t * tmp = list_entry(queue->prev,pcb_t,list);
    list_del_head(queue);
    return tmp;
}
