#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/task.h> //新增，用于初始化用户pcb
#include <os/string.h>
#include <os/kernel.h>
#include <os/smp.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
int pcb_flag[NUM_MAX_TASK] = {0}; //标记占用情况，方便回收
const ptr_t pid0_stack = INIT_KERNEL_STACK_0 + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack
};

const ptr_t pid1_stack = INIT_KERNEL_STACK_1 + PAGE_SIZE;
pcb_t pid1_pcb = {
    .pid = 0, //标记不回收
    .kernel_sp = (ptr_t)pid1_stack,
    .user_sp = (ptr_t)pid1_stack
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
pcb_t * volatile current_running_0;
pcb_t * volatile current_running_1;

/* global process id */
pid_t process_id = 1;
void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs
    // TODO: [p2-task1] Modify the current_running pointer.
    int cpu_id = get_current_cpu_id();
    current_running = (cpu_id == 0) ? current_running_0 : current_running_1;

    pcb_t * next_running;
    int isfirst=1;
    int changehead=1;
    int found=0; //找到合适的目标
    pcb_t * head;
    while(1){
        next_running = dequeue(&ready_queue);
        if(changehead==1){
            head=next_running;
            changehead=0;
        }
        if(next_running==NULL)
            break;
        else if(next_running==head && isfirst==0){
            enqueue(&ready_queue,next_running);
            break;
        }
        isfirst=0;
        if(next_running->status == TASK_EXITED)//抛弃 重取
        {
            if(next_running==head){
                changehead=1; 
                isfirst = 1;
            }
            pcb_flag[next_running->pid-1]=0; // 取消占用标记
        } 
        else if(next_running->status == TASK_READY && next_running->mask!=3 && next_running->mask != cpu_id+1)//重新入队
        {
            enqueue(&ready_queue,next_running);
        }
        else 
        {   
            found=1;
            break;
        }
    }
    //就绪队列为空时，next_running指定为初始进程
    if(found==0) {
        next_running = (cpu_id == 0) ? &pid0_pcb : &pid1_pcb;
        pcb_t * last_running = current_running;
        current_running = next_running;
        if(cpu_id == 0){
            current_running_0 = current_running;
        }
        else{
            current_running_1 = current_running;
        }
        if(last_running->status!=0 && last_running->status == TASK_EXITED)
            pcb_flag[last_running->pid-1] = 0; 
        else if(last_running->pid==0 || last_running->status == TASK_EXITED || last_running->status == TASK_BLOCKED)
            ;
        else
        {
            last_running->status = TASK_READY;
            enqueue(&ready_queue,last_running);
        }
        switch_to(last_running,next_running);
        return;            
    }    
        
    pcb_t * last_running;
    last_running = current_running;

    if(current_running->pid == 0)
    {
        //对于寻找到目标的初始进程，才可以设置定时器
        set_timer(get_ticks()+TIMER_INTERVAL);
    }

    if(current_running->status!=0 && current_running->status == TASK_EXITED)
        pcb_flag[current_running->pid-1]=0;
    else if(current_running->pid != 0 && current_running->status != TASK_BLOCKED && current_running->status !=TASK_EXITED){//task1中只需考虑pcb0不回收，后续任务需要考虑状态
        current_running->status = TASK_READY;
        enqueue(&ready_queue,current_running);
    }

    next_running->status = TASK_RUNNING;
    current_running = next_running;
    if(cpu_id == 0){
        current_running_0 = current_running;
    }
    else{
        current_running_1 = current_running;
    }
    switch_to(last_running,next_running);

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
        int coreid;
        if(pcb[i].pid == current_running_0->pid)
            coreid =0;
        else if(pcb[i].pid == current_running_1->pid)
            coreid =1;
        else
            coreid =2;//illegal
        printk("[%d] PID : %d PPID : %d STATUS : ",i,pcb[i].pid,pcb[i].ppid);
        switch(pcb[i].status){
            case TASK_BLOCKED: printk("BLOCKED mask:%d\n",pcb[i].mask); break;
            case TASK_RUNNING: printk("RUNNING mask:%d Running on Core %d\n",pcb[i].mask,coreid); break;
            case TASK_READY  : printk("READY   mask:%d\n",pcb[i].mask); break;
            case TASK_EXITED : printk("EXITED  mask:%d\n",pcb[i].mask); break;
            default: break;
        }
    }
}


pid_t do_exec(char *name, int argc, char *argv[]){
    char *shellptr = "shell";
    if(strcmp(name,shellptr)==0){
        printk("shell exists.\n");
        return 0;
    }
    pid_t pid = init_pcb(name,argc,argv);
    return pid; //不存在时返回0
}

void do_runmask(char *name, int argc, char *argv[],int mask){
    int pid = do_exec(name,argc,argv);
    do_setmask(pid,mask);
}
void do_setmask(int pid,int mask){
    if(pcb_flag[pid-1] == 0){
        printk("Err: Setmask Failed.\n");
    }
    else{
        pcb[pid-1].mask = mask;
    }
}
void pcb_recycle(pid_t pid){
    //功能：对应用户栈和内核栈取消占用标记，pcb块取消占用标记，释放等待队列，将属于其的锁队列和同步变量释放。调度
    //杀死子进程
    for(int i=0;i<NUM_MAX_TASK;i++){
        if(pcb_flag[i]==1 && pcb[i].ppid == pid)
            do_kill(pcb[i].pid);
    }
    //回收内存
    int Kernel_page_num = (pcb[pid-1].kernel_sp - FREEMEM_KERNEL) / PAGE_SIZE;
    int User_page_num = (pcb[pid-1].user_sp - FREEMEM_USER) / PAGE_SIZE;
    freeKernelPage(Kernel_page_num);
    freeUserPage(User_page_num);
    //释放锁队列
    for(int i=0;i<LOCK_NUM;i++)
    {
        if(mlocks[i].host_id == pid)
            do_mutex_lock_release(i);
    }
    //释放等待队列
    while(pcb[pid-1].wait_queue.prev != NULL){
        pcb_t * tmp = list_entry(pcb[pid-1].wait_queue.prev,pcb_t,list);
        dequeue(&(pcb[pid-1].wait_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
    //释放创建的屏障
    for(int i=0;i<BARRIER_NUM;i++){
        if(barr[i].used == 1 && barr[i].host_id == pid)
            do_barrier_destroy(i);
    }
    //释放创建的条件变量
    for(int i=0;i<CONDITION_NUM;i++){
        if(cond[i].used == 1 && cond[i].host_id == pid)
            do_condition_destroy(i);
    }
    //释放占用信箱
    for(int i=0;i<pcb[pid-1].mbox_cnt;i++){
        do_mbox_close(pcb[pid-1].mbox_arr[i]);
    }
    pcb[pid-1].mbox_cnt =0;
}
void do_exit(void){
    pid_t pid = current_running->pid;
    pcb_recycle(pid);
    current_running->status = TASK_EXITED;
    do_scheduler();
}
int do_kill(pid_t pid){
    //如果kill的是当前进程，则同exit一致。且此时无需关注返回值
    //否则需要更改其状态、回收资源，并在调度的时候取消PCB块占用
    //1.资源立即释放，防止过多占用PCB块。
        //e.g，子进程堵塞屏障中，父进程堵塞子进程等待队列。如果调度才释放，将持续占用PCB块
    //2.在调度时才更改标记，防止同时存在两个队列中
    if(pid == current_running->pid){
        do_exit();
        return 1;
    }
    else if(pcb_flag[pid-1] == 1){
        pcb[pid-1].status = TASK_EXITED;
        pcb_recycle(pid);
        return 1;
    }
    else 
        return 0;
}
int do_waitpid(pid_t pid){
    if(pcb_flag[pid-1] == 1){
        do_block(&(current_running->list),&(pcb[pid-1].wait_queue));
        do_scheduler();
        return pid;
    }
    else
    {
        return 0;
    }
}
pid_t do_getpid(){
    return current_running->pid;
}