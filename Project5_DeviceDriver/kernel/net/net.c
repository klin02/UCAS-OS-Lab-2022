#include <e1000.h>
#include <type.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>

static LIST_HEAD(send_block_queue);
static LIST_HEAD(recv_block_queue);

int do_net_send(void *txpacket, int length)
{
    // TODO: [p5-task1] Transmit one network packet via e1000 device
    // TODO: [p5-task3] Call do_block when e1000 transmit queue is full
    // TODO: [p5-task4] Enable TXQE interrupt if transmit queue is full
    //逐帧发送
    uint32_t frame_size = TX_PKT_SIZE;
    int sendbyte = 0;
    while(length > 0)
    {
        if(length < frame_size)
        {
            sendbyte += e1000_transmit(txpacket,length);
            break;
        }
        else{
            sendbyte += e1000_transmit(txpacket, frame_size);
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

    return 0;  // Bytes it has received
}

void net_handle_irq(void)
{
    // TODO: [p5-task4] Handle interrupts from network device
}