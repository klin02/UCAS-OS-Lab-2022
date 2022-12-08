#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <file.h>

static char buff[64];
#define gap (1<<20) //1MB
#define ROUND 8
char test[ROUND][20];
int main(void)
{
    sys_move_cursor(0,0);

    char databuf[10];
    char *testptr = "test ";
    for(int i=0;i<ROUND;i++){
        itoa(i,databuf,10,10);
        strcpy(test[i],testptr);
        strcat(test[i],databuf);
    }

    int fd = sys_fopen("2.txt", O_RDWR);

    for(int i=0;i<ROUND;i++){
        printf("Test: wr round %d\n",i);
        sys_fwrite(fd,test[i],strlen(test[i]));
        sys_lseek(fd,gap,SEEK_CUR,SEEK_WR);
    }

    for(int i=0;i<ROUND;i++){
        bzero(buff,64);
        printf("Test: rd round %d: ",i);
        sys_fread(fd,buff,strlen(test[i]));
        printf("%s\n",buff);
        sys_lseek(fd,gap,SEEK_CUR,SEEK_RD);
    }

    sys_fclose(fd);

    return 0;
}