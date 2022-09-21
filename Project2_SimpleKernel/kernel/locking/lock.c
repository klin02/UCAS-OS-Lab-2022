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
    //支持多把锁
    for(int i=0;i<TASK_MAXNUM;i++){
        for(int j=0;j<LOCK_NUM;j++)
            lock_map[i].lock_hit[j]=0; 
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

// int do_mutex_lock_init(int key)
// {
//     /* TODO: [p2-task2] initialize mutex lock */
//     //此处不需要额外进行锁的初始化，只需要根据key获取对应的锁id
//     //简单的一种key对应id的映射是直接取模
//     int lock_id = key % LOCK_NUM;
//     return lock_id;
// }

// int do_mutex_lock_init(int key)
// {
//     /* TODO: [p2-task2] initialize mutex lock */
//     //此处不需要额外进行锁的初始化，只需要根据key获取对应的锁id
//     //简单的一种key对应id的映射是直接取模
//     int lock_id = key % LOCK_NUM;
//     return lock_id;
// }

// void do_mutex_lock_acquire(int mlock_idx)
// {
//     /* TODO: [p2-task2] acquire mutex lock */
//     //如果锁已经被占用，需要直接调度
//     if(mlocks[mlock_idx].lock.status == LOCKED){
//         do_block(&(current_running->list),&(mlocks[mlock_idx].block_queue)); // 改变pcb状态和进队列均在此完成
//         do_scheduler();
//         //注意ready进队列条件，被阻塞的进程不会在ready queue中被反复调用
//     }
//     else{
//         mlocks[mlock_idx].lock.status = LOCKED;
//     }
// }

// void do_mutex_lock_release(int mlock_idx)
// {
//     /* TODO: [p2-task2] release mutex lock */
//     //单锁状态，需要将block queue中的任务都改为ready态，并加入 ready queue
//     while(mlocks[mlock_idx].block_queue.prev != NULL){
//         pcb_t * tmp = list_entry(mlocks[mlock_idx].block_queue.prev,pcb_t,list);
//         dequeue(&(mlocks[mlock_idx].block_queue));
//         do_unblock(&(tmp->list)); //改变状态及入队列
//     }
// }

//lockmap和currentrunning为全局变量，可以直接用
void do_mutex_lock_init(int key){
    //假设会占用key模及key模+1的锁
    int id1 = key % LOCK_NUM;
    int id2 = (key+1) % LOCK_NUM;
    int pid = current_running->pid;
    if(key == 43) printl("my pid %d\n",pid);
    lock_map[pid].lock_hit[id1]=1;
    lock_map[pid].lock_hit[id2]=1;
}

void do_mutex_lock_acquire()
{
    while(1){
    //如果taskhit中有一个被锁，就进入所有hit的block queue
    int pid = current_running->pid;
    int hit_arr[16];
    int hit_len=0;
    for(int i=0;i<LOCK_NUM;i++){
        if(lock_map[pid].lock_hit[i]==1){
            hit_arr[hit_len++]=i;
        }
    }//获取命中的锁序列
    int some_block=0; //有某把锁被占用
    for(int i=0;i<hit_len;i++){
        if(mlocks[hit_arr[i]].lock.status==LOCKED){
            some_block=1;
            break;
        }
    }
    if(some_block==1){
        //进入全部命中锁的阻塞队列
        for(int i=0;i<hit_len;i++){
            do_block(&(current_running->list),&(mlocks[hit_arr[i]].block_queue));
        }
        do_scheduler();
        //另外一方释放锁后，还需要继续申请
    }
    else{
        for(int i=0;i<hit_len;i++){
            mlocks[hit_arr[i]].lock.status = LOCKED;
        }
        break; //结束申请
    }
    }
}

void do_mutex_lock_release()
{
    int pid = current_running->pid;
    int hit_arr[16];
    int hit_len=0;
    for(int i=0;i<LOCK_NUM;i++){
        if(lock_map[pid].lock_hit[i]==1){
            hit_arr[hit_len++]=i;
        }
    }//获取命中的锁序列
    //tricky！
    //1.对每个队列，由于可能头出尾入，头部需要拿出做终止条件。
    //2.考虑不重复进入ready队列，因此对成功入ready栈的使用used记录，之后遍历如果为used，可以直接出队。
    //2.对每次队头对应的pcb，依次检查其他非命中锁的阻塞状态，如果仍是阻塞，需要再次入队。
    list_node_t * head; //队头标记 
    list_node_t * tmp;
    pcb_t * tmppcb;
    int tmppid;
    int isfirst;
    int used[LOCK_NUM]={0};
    int other_block;
    for(int i=0;i<hit_len;i++){
        head = mlocks[hit_arr[i]].block_queue.prev;
        isfirst = 1;
        while(1){
            tmp = mlocks[hit_arr[i]].block_queue.prev;
            if(tmp==NULL || (tmp==head && isfirst==0))
                break;
            isfirst =0;
            tmppcb = list_entry(tmp,pcb_t,list);
            tmppid = tmppcb->pid;
            if(used[tmppid] == 1 || tmppid == pid){
                //已经在先前的遍历中成功进入ready queue。或者是由于还未占用就进入该队列block queue的任务
                dequeue(&(mlocks[hit_arr[i]].block_queue));
            }
            else{
                other_block = 0;
                for(int j=0;j<LOCK_NUM;j++){ // 如果有未在hitarr中的锁仍被占用
                    if(lock_map[tmppid].lock_hit[j] == 0)
                        continue;
                    if(mlocks[j].lock.status == UNLOCKED)
                        continue;
                    other_block = 1; // 有锁被占用
                    for(int k=0;k<hit_len;k++){
                        if(hit_arr[k]==j){
                            other_block = 0; //标记的锁将被释放，取消标记
                            break;
                        }
                    }
                    if(other_block)
                        break;
                }
                if(tmppid ==4)
                printl("otherblock %d\n",other_block);
                if(other_block){//需要重新入队
                    dequeue(&(mlocks[hit_arr[i]].block_queue));
                    enqueue(&(mlocks[hit_arr[i]].block_queue),tmppcb);
                }
                else{//没有其他占用的锁，可以成功释放
                    used[tmppid] = 1;
                    dequeue(&(mlocks[hit_arr[i]].block_queue));
                    do_unblock(tmp); //包含改变状态以及入ready queue
                }
            }
        }
    }
    //处理完所有锁对应的队列后，进行锁的释放
    for(int i=0;i<hit_len;i++){
        mlocks[hit_arr[i]].lock.status = UNLOCKED;
    }
}