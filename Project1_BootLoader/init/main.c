#include <common.h>
#include <asm.h>
#include <os/bios.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <type.h>

#define VERSION_BUF 50

//16个任务地址为0x52000000~0x520fffff
//用户扇区拷贝到的内存位置
#define USER_INFO_ADDR 0x52200000

//用户栈帧相关部分
#define STACK_NUM 16
//存储空闲栈帧号和数据的位置
#define STACK_TAG_BASE 0x52300000
#define STACK_ADDR_BASE 0x52310000

//用户程序运行的栈地址基址
#define TASK_STACK_BASE 0x53000000
//分配的栈帧大小
#define TASK_STACK_SIZE 0x10000
int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

// Task info array
task_info_t tasks[TASK_MAXNUM];
short tasknum;

//additional: stack array for task switch
char stack_tag[STACK_NUM];    //0 free 1 occupied
long stack_addr[STACK_NUM];
static void init_stack_array(void){
    //assign stack frames for different running stack
    //better design is queue, but array is easier for assembly
    for(int i=0;i<STACK_NUM;i++){
        stack_tag[i]=0;
        stack_addr[i]=TASK_STACK_BASE+i*TASK_STACK_SIZE;
    }
    unsigned char *ptr = (unsigned int *)STACK_TAG_BASE;
    memcpy(ptr,stack_tag,STACK_NUM * sizeof(char));
    ptr = (unsigned int *)STACK_ADDR_BASE;
    memcpy(ptr,stack_addr,STACK_NUM * sizeof(long));
}

static int bss_check(void)
{
    for (int i = 0; i < VERSION_BUF; ++i)
    {
        if (buf[i] != 0)
        {
            return 0;
        }
    }
    return 1;
}

static void init_bios(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())BIOS_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    // 根据用户信息扇区获取（已加载到内存），只需加载选定程序即可，在load中实现
    unsigned char * ptr = (unsigned int *)USER_INFO_ADDR;
    ptr += 8;
    //tasknum is define in task.h
    memcpy(&tasknum,ptr,2);
    ptr += 2;
    for(int i=0;i<tasknum;i++){
        memcpy(&tasks[i],ptr,sizeof(task_info_t));
        ptr += sizeof(task_info_t);
    }
}

//task5: init task order
char batbuf[512]={0};
int bat_array[100];
int bat_ptr=0;
short bat_sz;
static void init_bat(void)
{
    unsigned char *ptr = (unsigned int *)(USER_INFO_ADDR);
    ptr += 5*sizeof(short);
    ptr += tasknum*sizeof(task_info_t);
    memcpy(&bat_sz,ptr,2);
    ptr+=2;
    memcpy(batbuf,ptr,bat_sz);
    int tmp=0;
    char tmpch;
    for(int i=0;i<bat_sz;i++){
        tmpch = batbuf[i];
        if(tmpch >= '0'&& tmpch<='9')
            tmp = tmp*10 + tmpch - '0';
        else{//此处判断条件因为buf最后一个不是数字
            bat_array[bat_ptr++]=tmp;
            tmp=0;
        }
    }
    bat_ptr--;
}

int main(void)
{
    // Check whether .bss section is set to zero
    int check = bss_check();

    // Init jump table provided by BIOS (ΦωΦ)
    init_bios();

    // Init task information (〃'▽'〃)
    init_task_info();

    // task5: init bat
    init_bat();

    // additional: init task stack 
    init_stack_array();

    // Output 'Hello OS!', bss check result and OS version
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int output_val_pos = 0;

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    
    for (int i = 0; i < sizeof(output_str); ++i)
    {
        buf[i] = output_str[i];
        if (buf[i] == '_')
        {
            buf[i] = output_val[output_val_pos++];
        }
    }

    bios_putstr("Hello OS!\n\r");
    bios_putstr(buf);

    // TODO[p1-task2] use jmptab to get info from screen and redisplay
        //jmptab is covered by bios func
        //!! note that when getchar haven't get a input, it will return EOF(-1)
    // int screen_ch;
    // while(1){
    //     while((screen_ch = bios_getchar())==-1);    //loop until getting a char
    //     bios_putchar(screen_ch);
    // }
    
    // TODO: Load tasks by either task id [p1-task3] or task name [p1-task4],
    //   and then execute them.
        //task3: load task by id
    //int taskid=0;

        //task4: load task by name 
    // int taskid = -1; //非法初值，用于判断名字是否合法
    // char taskstr[10]={0};
    // int strptr=0;
    // bios_putstr("Please input task name (end with a non-num and non-letter char):\n\r");
    // int ch;
    // while(1){
    //     taskid=-1;
    //     bzero(taskstr,10);
    //     strptr=0;
    //     while(1){
    //         while((ch = bios_getchar())==-1);    //loop until getting a char
    //         bios_putchar(ch);
    //         //only num and letter is allowed
    //         if((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z'))
    //             taskstr[strptr++]=ch;
    //         else
    //             break;
    //     }
    //     for(int i=0;i<tasknum;i++){
    //         if(strcmp(tasks[i].name,taskstr)==0){
    //             taskid = i;
    //         }
    //     }
    //     if(taskid == -1)
    //         bios_putstr("Illegal task name, please input again:\n\r");
    //     else break;
    // }
    // load_task_img(taskid);

        //task5: load task by bat_array
    bios_putstr("task5: call func by bat:\n\r");
    // bios_putstr(batbuf);
    bios_putstr("\t bat info:");
    for(int i=0;i<bat_sz-1;i++)
        bios_putchar(batbuf[i]);
    bios_putstr("\n\r");
    for(int i=0;i<bat_ptr;i++){
        if(bat_array[i] < 0 || bat_array[i] >= tasknum)
            bios_putstr("task id err!\n\r");
        else
        load_task_img(bat_array[i]);
    }
    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        asm volatile("wfi");
    }

    return 0;
}
