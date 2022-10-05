#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define MAXLEN 1000
#define REGION (MAXLEN/TD_NUM + (MAXLEN%TD_NUM!=0))
//全局变量，放置在数据段，可共享
int data[MAXLEN]={0};
#define TD_NUM 2
int td_id[TD_NUM]={0}; //防止传地址改变值
int td_flag[TD_NUM]={0}; //flag=1表示该进程已经完成工作
int td_sum[TD_NUM]={0};
int print_location=7;
int child_print_location=8; //子线程打印位置
void *calc_sum(void *arg){
        int tid=*(int *)arg;
        int head = tid*REGION;
        int tail = head+REGION;
        
        for(int i=head;i<tail&&i<MAXLEN;i++){
                td_sum[tid]+=data[i];
                sys_move_cursor(0,child_print_location);
                printf("> [Thread] Child Thread %d is working! %03d /%03d~%03d   ",tid,i,head,tail-1);
                //sys_yield();
        }
        td_flag[tid]=1; //标记该线程工作完成
        //设置死循环以避免内存回收
        while(1){
                sys_move_cursor(0,child_print_location);
                printf("> [Thread] Child Thread %d is done!                            ",tid);
                //asm volatile("wfi");
                //sys_yield();
        }
}
int main(void){
        sys_move_cursor(0, print_location);
        printf("> [TASK] Main Thread is initializing            ");
        //sys_yield();
        for(int i=0;i<MAXLEN;i++)
                data[i]=i+1;
        for(int i=0;i<TD_NUM;i++)
                td_id[i]=i;
        //填入完毕后再创建线程
        sys_move_cursor(0, print_location);
        printf("> [TASK] Main Thread is creating               ");
        //sys_yield();
        for(int i=0;i<TD_NUM;i++){
                sys_thread_create(calc_sum,&td_id[i]);
        }
        
        sys_move_cursor(0, print_location);
        printf("> [TASK] Main Thread is waiting               ");
        //sys_yield();
        int result=0;
        while(1){
                int flag=1; //假设完成
                for(int i=0;i<TD_NUM;i++)
                        if(td_flag[i]==0){
                                flag=0;
                                break;
                        }
                if(flag==1){//所有线程工作完毕
                        for(int i=0;i<TD_NUM;i++)
                                result += td_sum[i];
                        break;
                }
                //sys_yield();
        }
        //设置死循环以免退出
        while(1){
                sys_move_cursor(0,print_location);
                printf("> [TASK] Main Thread is done. Result: %d              ",result);
                //sys_yield();
        }
        return;
}