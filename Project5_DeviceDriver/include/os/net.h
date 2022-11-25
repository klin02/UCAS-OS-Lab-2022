#ifndef __INCLUDE_NET_H__
#define __INCLUDE_NET_H__

#include <os/list.h>
#include <type.h>

#define PKT_NUM 32

extern list_head send_block_queue;
extern list_head recv_block_queue;

void net_handle_irq(void);
int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens);
int do_net_send(void *txpacket, int length);
void check_net_send();
void check_net_recv();

#endif  // !__INCLUDE_NET_H__