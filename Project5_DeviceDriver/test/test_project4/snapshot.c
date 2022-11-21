#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define TESTNUM 11
void stophere();
int stop=0;
int main(){
        //系统调用生成可写正本
        char *snap_init = (char *)sys_snap_init();
        for(int i=0;i<TESTNUM;i++)
                snap_init[i] = 'x';
        sys_move_cursor(0,0);
        printf("Snapbase   . vaddr %lx paddr %lx",snap_init,sys_va2pa(snap_init));
                printf("    Data: ");
                for(int j=0;j<TESTNUM;j++)
                        printf("%c ",snap_init[j]);
        printf("\n");
        char *snapshot[TESTNUM];
        //测试十次快照，每两次更改一次正本
        for(int i=0;i<TESTNUM;i++)
        {
                // printf("round %d\n",i);
                snapshot[i] = (char *)sys_snap_shot((void *)snap_init);
                // printf("Snapshot %d. vaddr %lx paddr %lx\n",i,snapshot[i],sys_va2pa(snapshot[i]));
                // printf("    Data: ");
                // for(int j=0;j<TESTNUM;j++)
                //         printf("%c ",snapshot[i][j]);
                // printf("\n");
                if(i%2!=0)
                {//第n次测试向前n个数字写n
                        // stophere();
                        for(int j=0;j<=i;j++)
                                snap_init[j] = '0'+i;
                }
        }
        //展示每次快照虚地址，物理地址，内容
        for(int i=0;i<TESTNUM;i++)
        {
                printf("Snapshot %02d. vaddr %lx paddr %lx",i,snapshot[i],sys_va2pa(snapshot[i]));
                printf("    Data: ");
                for(int j=0;j<TESTNUM;j++)
                        printf("%c ",snapshot[i][j]);
                printf("\n");
        }
        return 0;
}
void stophere(){
        stop =1;
}