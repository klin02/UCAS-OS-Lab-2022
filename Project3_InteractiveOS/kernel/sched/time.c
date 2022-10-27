#include <os/list.h>
#include <os/sched.h>
#include <type.h>

//(type *)0强制转化为地址为0的type类型指针。下述宏定义获得了member成员相对于type指针入口的偏移
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//第一行将进行类型检查，确定ptr与member类型是否一致。第二行则是根据成员变量减去偏移值得到了容器值。
#define list_entry(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

uint64_t time_elapsed = 0;
uint64_t time_base = 0;

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

void check_sleeping(void)
{
    // TODO: [p2-task3] Pick out tasks that should wake up from the sleep queue
    // 由于可能再次入队，取当前队头做标记
    //!! 使用队头标记法保证遍历的条件是，队头再入队。如果队头不入队，第二个重复入队，则陷入死循环。
    //因此，当队头出队时，需要更改头。
    list_node_t * head; //队头标记 
    list_node_t * tmp;
    pcb_t * tmppcb;
    int isfirst=1;
    int changehead = 1;
    uint64_t cur_time = get_timer();
    while(1){
        tmp = sleep_queue.prev;
        if(changehead==1){
            head=tmp;
            changehead=0;
        }
        if(tmp==NULL || (tmp==head && isfirst==0))
            break;
        isfirst=0;
        tmppcb = list_entry(tmp,pcb_t,list);
        if(tmppcb->wakeup_time <= cur_time){
            if(tmp==head)
                changehead=1;
            dequeue(&sleep_queue);
            do_unblock(&(tmppcb->list)); //需要改变状态再入队
        }
        else{
            dequeue(&sleep_queue);
            enqueue(&sleep_queue,tmppcb);
        }
    }
}