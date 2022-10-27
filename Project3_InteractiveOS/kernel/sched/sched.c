#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/task.h> //新增，用于初始化用户pcb
#include <os/string.h>
#include <os/kernel.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
int pcb_flag[NUM_MAX_TASK] = {0}; //标记占用情况，方便回收
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

//(type *)0强制转化为地址为0的type类型指针。下述宏定义获得了member成员相对于type指针入口的偏移
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//第一行将进行类型检查，确定ptr与member类型是否一致。第二行则是根据成员变量减去偏移值得到了容器值。
#define list_entry(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })


/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs
    //check_sleeping();
    //printl("begin schedule\n");
    // TODO: [p2-task1] Modify the current_running pointer.
    //将cur加入ready queue。同时从中拿出next
    pcb_t * next_running;
    next_running = dequeue(&ready_queue);
    //考虑就绪队列中已经被杀死的进程：回收，重新取
    while( next_running !=NULL && next_running->status == TASK_EXITED){
        pid_t nextpid = next_running->pid;
        pcb_recycle(nextpid);
        //重取
        next_running = dequeue(&ready_queue);
    }
    //就绪队列为空时，返回空，next需要指向自己
    if(next_running == NULL)     
        next_running = current_running;
    pcb_t * last_running;
    last_running = current_running;

    if(current_running->pid != 0 && current_running->status != TASK_BLOCKED && current_running->status !=TASK_EXITED){//task1中只需考虑pcb0不回收，后续任务需要考虑状态
        current_running->status = TASK_READY;
        enqueue(&ready_queue,current_running);
    }

    next_running->status = TASK_RUNNING;
    current_running = next_running;
    // TODO: [p2-task1] switch_to current_running
    set_timer(get_ticks()+TIMER_INTERVAL);
    switch_to(last_running,current_running);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
    current_running->wakeup_time = get_timer() + sleep_time;
    do_block(&(current_running->list),&sleep_queue); //改变状态及入队列
    do_scheduler(); //状态改变，不进ready queue。 通过do_scheduler开头的timechech唤醒
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: [p2-task2] block the pcb task into the block queue
    pcb_t *pcb = list_entry(pcb_node,pcb_t,list);
    pcb->status = TASK_BLOCKED;
    enqueue(queue,pcb);
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
    //完成改变状态和入ready队列
    pcb_t *pcb = list_entry(pcb_node,pcb_t,list);
    if(pcb->status != TASK_EXITED)
        pcb->status = TASK_READY;
    enqueue(&ready_queue,pcb);
    // if(pcb->pid == 4)
    //     printl("unblock my\n");
}

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

void do_process_show(){
    printk("[Process Table]:\n");
    for(int i=0;i<NUM_MAX_TASK;i++){
        if(pcb_flag[i]==0)
            continue;
        printk("[%d] PID : %d  STATUS : ",i,pcb[i].pid);
        switch(pcb[i].status){
            case TASK_BLOCKED: printk("BLOCKED\n"); break;
            case TASK_RUNNING: printk("RUNNING\n"); break;
            case TASK_READY  : printk("READY\n");   break;
            case TASK_EXITED : printk("EXITED\n");  break;
            default: break;
        }
    }
}


pid_t do_exec(char *name, int argc, char *argv[]){
    //printk("stophere;");
    //printk("name:%s tasknum:%d\n",name,tasknum);
    //printk("size:%d \n",sizeof(argv[0]));
    char *shellptr = "shell";
    if(strcmp(name,shellptr)==0){
        printk("shell is exited.\n");
        return 0;
    }
    pid_t pid = init_pcb(name,argc,argv);
    //do_scheduler();
    //printk("pid=%d\n",pid);
    return pid; //不存在时返回0
    //do_process_show();
    //while(1);
}

void pcb_recycle(pid_t pid){
    //功能：对应用户栈和内核栈取消占用标记，pcb块取消占用标记，释放等待队列，将属于其的锁队列释放。调度
    //回收内存
    int Kernel_page_num = (pcb[pid-1].kernel_sp - FREEMEM_KERNEL) / PAGE_SIZE;
    int User_page_num = (pcb[pid-1].user_sp - FREEMEM_USER) / PAGE_SIZE;
    freeKernelPage(Kernel_page_num);
    freeUserPage(User_page_num);
    //回收tcb块
    pcb_flag[pid -1] = 0;
    //释放锁队列
    for(int i=0;i<LOCK_NUM;i++)
    {
        if(mlocks[i].owner_pid == pid)
            do_mutex_lock_release(i);
    }
    //释放等待队列
    while(pcb[pid-1].wait_queue.prev != NULL){
        pcb_t * tmp = list_entry(pcb[pid-1].wait_queue.prev,pcb_t,list);
        dequeue(&(pcb[pid-1].wait_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
}
void do_exit(void){
    pid_t pid = current_running->pid;
    pcb_recycle(pid);
    current_running->status = TASK_EXITED;
    do_scheduler();
}
int do_kill(pid_t pid){
    //如果kill的是当前进程，则同exit一致。且此时无需关注返回值
    //否则需要更改其状态，并在调度或者释放锁的时候进行相应处理
    if(pid == current_running->pid){
        do_exit();
        return 1;
    }
    else if(pcb_flag[pid-1] == 1){
        pcb[pid-1].status = TASK_EXITED;
        return 1;
    }
    else 
        return 0;
}
int do_waitpid(pid_t pid){
    if(pcb_flag[pid-1] == 1){
        do_block(&(current_running->list),&(pcb[pid-1].wait_queue));
        if(current_running->status == TASK_BLOCKED) return pid;
        do_scheduler();
        return pid;
    }
    else
    {
        printk("err\n");
        return 0;
    }
}
pid_t do_getpid(){
    return current_running->pid;
}