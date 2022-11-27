#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stddef.h>

#define PKT_QUEUE 128
#define PKT_SIZE  128

uint32_t pkt_buf[PKT_QUEUE][PKT_SIZE];
int pkt_len[PKT_QUEUE];
int head;
int tail = 0;
int recv_print_loc = 1;
int send_print_loc = 2;
void* send_pkt();
void* recv_pkt();
int main(void)
{
    pthread_t recv;
    pthread_create(&recv, send_pkt, NULL);
    recv_pkt();
    pthread_join(recv);
    return 0;
}
void* send_pkt(){
        //init
        int send_cnt = 0;
        uint32_t *addr[PKT_QUEUE];
        for(int i=0;i<PKT_QUEUE;i++)
                addr[i] = pkt_buf[i];
        while(1){
                if(tail == head)
                        continue;
                else{
                        char *ptr;
                        char *base = (char*)addr[tail];
                        for(ptr=base;ptr<base+6;ptr++)
                                *ptr = 0xff;
                        // for(ptr=base+pkt_len[tail];ptr>base+64;ptr--)
                        //         *ptr = *(ptr-1);
                        ptr = base+54;
                        *(ptr++) = 'R';
                        *(ptr++) = 'e';
                        *(ptr++) = 's';
                        *(ptr++) = 'p';
                        *(ptr++) = 'o';
                        *(ptr++) = 'n';
                        *(ptr++) = 's';
                        *(ptr++) = 'e';
                        *(ptr++) = ':';
                        *(ptr++) = ' ';
                        sys_net_send(addr[tail],pkt_len[tail]);
                        tail = (tail+1)%PKT_QUEUE;
                        sys_move_cursor(0,send_print_loc);
                        send_cnt++;
                        printf("[SEND] PKT num : %d\n",send_cnt);
                }
        }
}
void* recv_pkt(){
        int recv_cnt = 0;
        int recv_length[1];
        while(1){
                sys_net_recv(pkt_buf[head],1,recv_length);
                pkt_len[head] = recv_length[0];
                head = (head+1)%PKT_QUEUE;
                if(head == tail)
                {
                        printf(">Err: recv: head = tail: %d\n",head);
                        while(1);
                }
                sys_move_cursor(0,recv_print_loc);
                recv_cnt++;
                printf("[RECV] PKT num : %d\n",recv_cnt);
                // sys_sleep(1);
        }
}