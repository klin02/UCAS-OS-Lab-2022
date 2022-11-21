#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <os/string.h>
#include <atomic.h>
#include <printk.h>


mutex_lock_t mlocks[LOCK_NUM];
barrier_t    barr[BARRIER_NUM];
condition_t  cond[CONDITION_NUM];
mailbox_t    mbox[MBOX_NUM];

//(type *)0强制转化为地址为0的type类型指针。下述宏定义获得了member成员相对于type指针入口的偏移
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//第一行将进行类型检查，确定ptr与member类型是否一致。第二行则是根据成员变量减去偏移值得到了容器值。
#define list_entry(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })


void init_locks(void)
{
    /* TODO: [p2-task2] initialize mlocks */
    //需要对所有锁进行初始化
    for(int i=0;i<LOCK_NUM;i++){
        mlocks[i].lock.status = UNLOCKED;
        list_init(&(mlocks[i].block_queue));
        mlocks[i].key = i;
        mlocks[i].host_pid = 0;
        mlocks[i].host_tid = 0;
    }
    //支持多把锁
    // for(int i=0;i<TASK_MAXNUM;i++){
    //     for(int j=0;j<LOCK_NUM;j++)
    //         lock_map[i].lock_hit[j]=0; 
    // }
}

void spin_lock_init(spin_lock_t *lock)
{
    /* TODO: [p2-task2] initialize spin lock */
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] try to acquire spin lock */
    return 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] acquire spin lock */
}

void spin_lock_release(spin_lock_t *lock)
{
    /* TODO: [p2-task2] release spin lock */
}

int do_mutex_lock_init(int key)
{
    /* TODO: [p2-task2] initialize mutex lock */
    //此处不需要额外进行锁的初始化，只需要根据key获取对应的锁id
    //简单的一种key对应id的映射是直接取模
    int lock_id = key % LOCK_NUM;
    return lock_id;
}

void do_mutex_lock_acquire(int mlock_idx)
{
    /* TODO: [p2-task2] acquire mutex lock */
    //如果锁已经被占用，需要直接调度
    while(1){
    if(mlocks[mlock_idx].lock.status == LOCKED){
        do_block(&(current_running->list),&(mlocks[mlock_idx].block_queue)); // 改变pcb状态和进队列均在此完成
        do_scheduler();
        //注意ready进队列条件，被阻塞的进程不会在ready queue中被反复调用
    }
    else{
        mlocks[mlock_idx].lock.status = LOCKED;
        mlocks[mlock_idx].host_pid = current_running->pid;
        mlocks[mlock_idx].host_tid = current_running->tid;
        break;
    }
    }
}
//判断是否要加上ROB
void do_mutex_lock_release(int mlock_idx)
{
    /* TODO: [p2-task2] release mutex lock */
    //单锁状态，需要将block queue中的任务都改为ready态，并加入 ready queue
    while(mlocks[mlock_idx].block_queue.prev != NULL){
        pcb_t * tmp = list_entry(mlocks[mlock_idx].block_queue.prev,pcb_t,list);
        dequeue(&(mlocks[mlock_idx].block_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
    mlocks[mlock_idx].lock.status = UNLOCKED;
    mlocks[mlock_idx].host_pid = 0;
    mlocks[mlock_idx].host_tid = 0;
}

void init_barriers(void){
    for(int i=0;i<BARRIER_NUM;i++){
        barr[i].total_num = 0;
        barr[i].wait_num  = 0;
        list_init(&barr[i].wait_queue);
        barr[i].used = 0;
    }
}

int do_barrier_init(int key, int goal){
    int bar_idx = key % BARRIER_NUM;
    if(barr[bar_idx].used == 1){
        printk("Barr %d is used\n",bar_idx);
        return -1; //返回非法值
    }
    else{
        //相关量均已在destroy时重置
        barr[bar_idx].total_num = goal;
        barr[bar_idx].used = 1;
        barr[bar_idx].host_pid = current_running->pid;
        barr[bar_idx].host_tid = current_running->tid;
        barr[bar_idx].wait_num =0;
        list_init(&barr[bar_idx].wait_queue);
        return bar_idx;
    }
}

void do_barrier_wait(int bar_idx){
    if(barr[bar_idx].used == 0){
        printk("Barr %d does not exist\n",bar_idx);
    }
    else{
        barr[bar_idx].wait_num ++;
        if(barr[bar_idx].wait_num == barr[bar_idx].total_num){
            //barr[bar_idx].wait_num--; //最新的不入
            while(barr[bar_idx].wait_queue.prev != NULL){
                pcb_t * tmp = list_entry(barr[bar_idx].wait_queue.prev,pcb_t,list);
                dequeue(&(barr[bar_idx].wait_queue));
                do_unblock(&(tmp->list)); //改变状态及入队列
                //barr[bar_idx].wait_num--;
            }
            barr[bar_idx].wait_num=0;
            do_scheduler();
        }
        else{
            //printk("                    block");
            do_block(&(current_running->list),&(barr[bar_idx].wait_queue));
            //do_process_show();
            do_scheduler();
        }
    }
}

void do_barrier_destroy(int bar_idx){
    if(barr[bar_idx].used == 0){
        printk("Barr %d does not exist\n",bar_idx);
    }
    else{
        barr[bar_idx].total_num =0;
        while(barr[bar_idx].wait_queue.prev != NULL){
                pcb_t * tmp = list_entry(barr[bar_idx].wait_queue.prev,pcb_t,list);
                dequeue(&(barr[bar_idx].wait_queue));
                do_unblock(&(tmp->list)); //改变状态及入队列
                barr[bar_idx].wait_num--;
            }
        barr[bar_idx].used=0;
        barr[bar_idx].host_pid =0;
        barr[bar_idx].host_tid =0;
    }
}

void init_conditions(void){
    for(int i=0;i<CONDITION_NUM;i++){
        list_init(&cond[i].wait_queue);
        cond[i].used=0;
    }
}
int do_condition_init(int key){
    int cid = key % CONDITION_NUM;
    if(cond[cid].used ==1)
    {
        printk("Cond %d is used",cid);
        return -1;
    }
    else{
        cond[cid].used =1;
        cond[cid].host_pid = current_running->pid;
        cond[cid].host_tid = current_running->tid;
        return cid;
    }
}
void do_condition_wait(int cond_idx, int mutex_idx){
    do_block(&(current_running->list),&(cond[cond_idx].wait_queue));
    do_mutex_lock_release(mutex_idx);
    do_scheduler();
    do_mutex_lock_acquire(mutex_idx);
}
void do_condition_signal(int cond_idx){
    if(cond[cond_idx].wait_queue.prev != NULL){
        pcb_t * tmp = list_entry(cond[cond_idx].wait_queue.prev,pcb_t,list);
        dequeue(&(cond[cond_idx].wait_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
}
void do_condition_broadcast(int cond_idx){
    while(cond[cond_idx].wait_queue.prev != NULL){
        pcb_t * tmp = list_entry(cond[cond_idx].wait_queue.prev,pcb_t,list);
        dequeue(&(cond[cond_idx].wait_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
}
void do_condition_destroy(int cond_idx){
    while(cond[cond_idx].wait_queue.prev != NULL){
        pcb_t * tmp = list_entry(cond[cond_idx].wait_queue.prev,pcb_t,list);
        dequeue(&(cond[cond_idx].wait_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
    cond[cond_idx].used = 0;
    cond[cond_idx].host_pid = 0;
    cond[cond_idx].host_tid = 0;
}

void init_mbox(){
    for(int i=0;i<MBOX_NUM;i++){
        bzero(mbox[i].name,20);
        bzero(mbox[i].buf,MAX_MBOX_LENGTH);
        mbox[i].status = CLOSED;
        mbox[i].index =0;
        mbox[i].visitor =0;
        list_init(&mbox[i].full_queue);
        list_init(&mbox[i].empty_queue);
    }
}
int do_mbox_open(char *name){
    //首先遍历，如果已经打开，直接获取，否则创建
    for(int i=0;i<MBOX_NUM;i++){
        if(mbox[i].status == OPEN && strcmp(mbox[i].name,name)==0){
            //记录该进程占用的信箱
            current_running->mbox_arr[current_running->mbox_cnt++] = i;
            mbox[i].visitor++;
            return i;
        }
    }
    //未找到，则创建
    for(int i=0;i<MBOX_NUM;i++){
        if(mbox[i].status == CLOSED)
        {
            strcpy(mbox[i].name,name);
            mbox[i].status = OPEN;
            current_running->mbox_arr[current_running->mbox_cnt++] = i;
            mbox[i].visitor++;
            //buf index queue都应该由初始化或回收完成
            return i;
        }
    }
    printk("Err: All mbox are used\n");
    return -1;
}
void do_mbox_close(int mbox_idx){
    mbox[mbox_idx].visitor --;
    if(mbox[mbox_idx].visitor==0){
        //全部访问进程都结束
        bzero(mbox[mbox_idx].name,20);
        bzero(mbox[mbox_idx].buf,MAX_MBOX_LENGTH);
        mbox[mbox_idx].status = CLOSED;
        mbox[mbox_idx].index =0;
        mbox[mbox_idx].visitor =0;
    while(mbox[mbox_idx].empty_queue.prev != NULL){
        pcb_t * tmp = list_entry(mbox[mbox_idx].empty_queue.prev,pcb_t,list);
        dequeue(&(mbox[mbox_idx].empty_queue));
        do_unblock(&(tmp->list)); //保持EXITED状态
    }
    while(mbox[mbox_idx].full_queue.prev != NULL){
        pcb_t * tmp = list_entry(mbox[mbox_idx].full_queue.prev,pcb_t,list);
        dequeue(&(mbox[mbox_idx].full_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
        // list_init(&mbox[mbox_idx].full_queue);
        // list_init(&mbox[mbox_idx].empty_queue);
        //printk("                                     Close Mbox suc!");
    }
}
int do_mbox_send(int mbox_idx, void * msg, int msg_length){
    if(mbox[mbox_idx].status == CLOSED){
        printk("Err: mbox %d does no exits;");
        return 0; //发送失败
    }
    int blocked =0; //统计被堵塞的次数
    while(mbox[mbox_idx].index + msg_length >= MAX_MBOX_LENGTH){ //注意循环判断
        do_block(&(current_running->list),&(mbox[mbox_idx].full_queue));
        do_scheduler();
        blocked++;
    }
    //可填充数据
    for(int i=0;i<msg_length;i++)
    {
        mbox[mbox_idx].buf[mbox[mbox_idx].index++] = ((char *)msg)[i];
    }
    //清空empty队列
    while(mbox[mbox_idx].empty_queue.prev != NULL){
        pcb_t * tmp = list_entry(mbox[mbox_idx].empty_queue.prev,pcb_t,list);
        dequeue(&(mbox[mbox_idx].empty_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
    return blocked;
}
int do_mbox_recv(int mbox_idx, void * msg, int msg_length){
    if(mbox[mbox_idx].status == CLOSED){
        printk("Err: mbox %d does no exits;");
        return 0; //发送失败
    }
    int blocked =0;
    while(mbox[mbox_idx].index < msg_length){
        do_block(&(current_running->list),&(mbox[mbox_idx].empty_queue));
        do_scheduler();
        blocked++;
    }
    //可提取数据
    //从头部拿出数据，并将剩余位置前移
    for(int i=0;i<msg_length;i++)
        ((char *)msg)[i] = mbox[mbox_idx].buf[i];
    for(int i=0;i<MAX_MBOX_LENGTH-msg_length;i++)
        mbox[mbox_idx].buf[i] = mbox[mbox_idx].buf[i+msg_length];
    mbox[mbox_idx].index -= msg_length;
    //清空full队列
    while(mbox[mbox_idx].full_queue.prev != NULL){
        pcb_t * tmp = list_entry(mbox[mbox_idx].full_queue.prev,pcb_t,list);
        dequeue(&(mbox[mbox_idx].full_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
    return blocked;
}