#include <e1000.h>
#include <type.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>
#include <assert.h>

//(type *)0强制转化为地址为0的type类型指针。下述宏定义获得了member成员相对于type指针入口的偏移
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//第一行将进行类型检查，确定ptr与member类型是否一致。第二行则是根据成员变量减去偏移值得到了容器值。
#define list_entry(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

LIST_HEAD(send_block_queue);
LIST_HEAD(recv_block_queue);

//堵塞处理流程：
    //1. 使能相关处理中断
    //2. 堵塞当前状态并调度
int do_net_send(void *txpacket, int length)
{
    // TODO: [p5-task1] Transmit one network packet via e1000 device
    // TODO: [p5-task3] Call do_block when e1000 transmit queue is full
    // TODO: [p5-task4] Enable TXQE interrupt if transmit queue is full
    //逐帧发送
    uint32_t frame_size = TX_PKT_SIZE;
    int sendbyte = 0;
    int tmp;
    while(length > 0)
    {
        if(length < frame_size)
        {
            while(1){
                tmp = e1000_transmit(txpacket,length);
                if(tmp == 0){
                    e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
                    local_flush_dcache();
                    do_block(&(current_running->list),&send_block_queue);
                    do_scheduler();                    
                }
                else //send successfully
                    break;
            }
            sendbyte += tmp;
            break;
        }
        else{
            while(1){
                tmp = e1000_transmit(txpacket,frame_size);
                if(tmp ==0){
                    e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
                    local_flush_dcache();
                    do_block(&(current_running->list),&send_block_queue);
                    do_scheduler();
                }
                else 
                    break;
            }
            sendbyte += tmp;
            txpacket += frame_size;
            length -= frame_size;
        }
    }
    return sendbyte;  // Bytes it has transmitted
}

int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens)
{
    // TODO: [p5-task2] Receive one network packet via e1000 device
    // TODO: [p5-task3] Call do_block when there is no packet on the way
    uint32_t frame_size = RX_PKT_SIZE;
    int recvbyte = 0;
    int tmp;
    for(int i=0;i<pkt_num;i++){
        while(1){
            tmp = e1000_poll(rxbuffer);
            if(tmp == 0)
            {
                e1000_write_reg(e1000, E1000_IMS, E1000_IMS_RXDMT0);
                local_flush_dcache();
                do_block(&(current_running->list),&recv_block_queue);
                do_scheduler();
            }
            else
                break;
        }
        pkt_lens[i] = tmp;
        recvbyte += tmp;
        rxbuffer += tmp;
    }
    return recvbyte;  // Bytes it has received
}

void check_net_send(){
    local_flush_dcache();
    uint32_t head = e1000_read_reg(e1000, E1000_TDH);
    uint32_t tail = e1000_read_reg(e1000, E1000_TDT);
    if( (tail+1)%TXDESCS != head)
    {
        while(send_block_queue.prev != NULL){
            pcb_t * tmp = list_entry(send_block_queue.prev,pcb_t,list);
            dequeue(&send_block_queue);
            do_unblock(&(tmp->list)); //改变状态及入队列
        }
    }
    else
        return ;
}

void check_net_recv(){
    local_flush_dcache();
    uint32_t head = e1000_read_reg(e1000, E1000_RDH);
    uint32_t tail = e1000_read_reg(e1000, E1000_RDT);
    if( (tail+1)%RXDESCS != head)
    {
        while(recv_block_queue.prev != NULL){
            pcb_t * tmp = list_entry(recv_block_queue.prev,pcb_t,list);
            dequeue(&recv_block_queue);
            do_unblock(&(tmp->list)); //改变状态及入队列
        }
    }
    else
        return ;
}

void net_handle_irq(void)
{
    // TODO: [p5-task4] Handle interrupts from network device
    //处理流程
        //1.读取类型
        //2.禁用中断
        //3.进行处理
    // printk("Enter net irq!\n");
    local_flush_dcache();
    uint32_t icr_val = e1000_read_reg(e1000, E1000_ICR);
    uint32_t is_txqe = icr_val & E1000_ICR_TXQE;
    uint32_t is_rxcmt0 = icr_val & E1000_ICR_RXDMT0;
    int do_handle= 0;
    if(is_txqe != 0)
    {
        do_handle ++;
        // printk(">>> irq txqe\n");
        // uint32_t old_ims = e1000_read_reg(e1000, E1000_IMS);
        e1000_write_reg(e1000, E1000_IMC, E1000_IMC_TXQE);
        local_flush_dcache();
        // latency(1);
        // uint32_t new_ims = e1000_read_reg(e1000, E1000_IMS);
        // printk(">>>> old ims %x new ims %x\n",old_ims,new_ims);
        check_net_send();
        // printk(">>> end txqe\n");
    }
    if(is_rxcmt0 != 0)
    {
        do_handle++;
        // printk("irq rxdmt0\n");
        e1000_write_reg(e1000, E1000_IMC, E1000_IMC_RXDMT0);
        local_flush_dcache();
        check_net_recv();
    }
    if(do_handle == 0)
    {
        printk("Err: net handle %x unknown!\n",icr_val);
        assert(0);
    }
}