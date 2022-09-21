#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>

mutex_lock_t mlocks[LOCK_NUM];
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
    }
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
    if(mlocks[mlock_idx].lock.status == LOCKED){
        do_block(&(current_running->list),&(mlocks[mlock_idx].block_queue)); // 改变pcb状态和进队列均在此完成
        do_scheduler();
        //注意ready进队列条件，被阻塞的进程不会在ready queue中被反复调用
    }
    else{
        mlocks[mlock_idx].lock.status = LOCKED;
    }
}

void do_mutex_lock_release(int mlock_idx)
{
    /* TODO: [p2-task2] release mutex lock */
    //单锁状态，需要将block queue中的任务都改为ready态，并加入 ready queue
    while(mlocks[mlock_idx].block_queue.prev != NULL){
        pcb_t * tmp = list_entry(mlocks[mlock_idx].block_queue.prev,pcb_t,list);
        dequeue(&(mlocks[mlock_idx].block_queue));
        do_unblock(&(tmp->list)); //改变状态及入队列
    }
}
