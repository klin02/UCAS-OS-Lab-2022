#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <os/string.h>
#include <atomic.h>

mutex_lock_t mlocks[LOCK_NUM];
barrier_t    barr[BARRIER_NUM];
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
        mlocks[i].owner_pid = 0;
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
        mlocks[mlock_idx].owner_pid = current_running->pid;
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
    mlocks[mlock_idx].owner_pid = 0;
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
        // printk("init: %d %d %d \n",barr[bar_idx].total_num,barr[bar_idx].wait_num,bar_idx);
        // while(1) ;
        if(barr[bar_idx].wait_num != 0) printk("Err1: barr is not empty\n");
        return bar_idx;
    }
}

void do_barrier_wait(int bar_idx){
    if(barr[bar_idx].used == 0){
        printk("Barr %d does not exist\n",bar_idx);
    }
    else{
        // printk("Info: %d %d %d\n",barr[bar_idx].wait_num,barr[bar_idx].total_num,bar_idx);
        // int bef = barr[bar_idx].wait_num;
        if(barr[bar_idx].wait_num ==0){
            if(barr[bar_idx].wait_queue.prev == NULL && barr[bar_idx].wait_queue.next == NULL)
            ;
            else 
                printk("ERR: list is not empty\n");
        }
        barr[bar_idx].wait_num ++;
        if(barr[bar_idx].wait_num == barr[bar_idx].total_num){
            //barr[bar_idx].wait_num--; //最新的不入
            while(barr[bar_idx].wait_queue.prev != NULL){
                pcb_t * tmp = list_entry(barr[bar_idx].wait_queue.prev,pcb_t,list);
                dequeue(&(barr[bar_idx].wait_queue));
                do_unblock(&(tmp->list)); //改变状态及入队列
                barr[bar_idx].wait_num--;
            }
            //if(barr[bar_idx].wait_num != 0) printk("Err2: barr is not empty %d %d %d %d!!\n",barr[bar_idx].wait_num,barr[bar_idx].total_num,bef,bar_idx);
            barr[bar_idx].wait_num=0;
        }
        else{
            do_block(&(current_running->list),&(barr[bar_idx].wait_queue));
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
        if(barr[bar_idx].wait_num != 0) printk("Err3: barr is not empty\n");
        barr[bar_idx].used=0;
    }
}