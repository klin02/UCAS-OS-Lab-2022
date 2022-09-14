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
char batbuf1[512]={0};
char batbuf2[512]={0};
int bat_id_array[100];
char bat_name_array[20][20]; //num and length
int bat_id_ptr=0;
int bat_name_ptr=0;
short bat_sz1;
short bat_sz2;
static void init_bat(void)
{
    unsigned char *ptr = (unsigned int *)(USER_INFO_ADDR);
    ptr += 5*sizeof(short);
    ptr += tasknum*sizeof(task_info_t);
    memcpy(&bat_sz1,ptr,2);
    ptr+=2;
    memcpy(&bat_sz2,ptr,2);
    ptr+=2;
    memcpy(batbuf1,ptr,bat_sz1);
    ptr+=bat_sz1;
    memcpy(batbuf2,ptr,bat_sz2);

    int tmp=0;
    char tmpch;
    for(int i=0;i<bat_sz1;i++){
        tmpch = batbuf1[i];
        if(tmpch >= '0'&& tmpch<='9')
            tmp = tmp*10 + tmpch - '0';
        else{//此处判断条件因为buf最后一个不是数字
            bat_id_array[bat_id_ptr++]=tmp;
            tmp=0;
        }
    }
    bat_id_ptr--;

    char tmpstr[20];
    int tmpstr_ptr=0;
    for(int i=0;i<bat_sz2;i++){
        tmpch = batbuf2[i];
        if((tmpch>='0'&&tmpch<='9') || (tmpch>='a'&&tmpch<='z') || (tmpch>='A'&&tmpch<='Z'))
            tmpstr[tmpstr_ptr++] = tmpch;
        else{
            tmpstr[tmpstr_ptr] = '\0';
            memcpy(bat_name_array[bat_name_ptr++],tmpstr,20);
            tmpstr_ptr =0;
        }
    }
    bat_name_ptr--;
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

    int func;
    bios_putstr("Press 0: run task by id\n\r");
    bios_putstr("Press 1: run task by name\n\r");
    bios_putstr("Press 2: run task by bat1.txt(id-order)\n\r");
    bios_putstr("Press 3: run task by bat2.txt(name-order)\n\r");
    while(1){
        while((func = bios_getchar())==-1);    //loop until getting a char
        bios_putchar(func);
        //only num and letter is allowed
        if(func>='0' && func<='3'){
            bios_putstr("\n\r");
            break;
        } 
        else
            bios_putstr("\n\rIllegal func! Input again!\n\r");
    }
    int ch;
    int taskid = -1; //非法初值，用于判断名字是否合法
    char taskstr[10]={0};
    int strptr;
    if(func == '0'){
        bios_putstr("[Func 0] Run task by id, please input task id:\n\r");
        int input_id;
        int valid_tag; //防止未输入默认为0
        while(1){
            input_id = 0;
            valid_tag =0;
            while(1){
                while((ch = bios_getchar())==-1);    //loop until getting a char
                bios_putchar(ch);
                //only num and letter is allowed
                if(ch>='0' && ch<='9'){
                    valid_tag =1;
                    input_id = input_id*10 + (ch-'0');
                }
                else
                    break;
            }
            if(input_id<0 || input_id>=tasknum || valid_tag==0)
                bios_putstr("\n\rIllegal task id! Input again!\n\r");
            else{
                bios_putstr("\n\r");
                load_task_img(input_id);
            }
            bios_putstr("Please input task id:\n\r");
        }
    }
    else if(func == '1'){
        bios_putstr("[Func1] Run task by name, please input task name:\n\r");
        strptr=0;
        while(1){
            taskid=-1;
            bzero(taskstr,10);
            strptr=0;
            while(1){
                while((ch = bios_getchar())==-1);    //loop until getting a char
                bios_putchar(ch);
                //only num and letter is allowed
                if((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z'))
                    taskstr[strptr++]=ch;
                else
                    break;
            }
            for(int i=0;i<tasknum;i++){
                if(strcmp(tasks[i].name,taskstr)==0){
                    taskid = i;
                }
            }
            if(taskid == -1)
                bios_putstr("Illegal task name, please input again:\n\r");
            else{
                bios_putstr("\n\r");
                load_task_img(taskid);
            }
            bios_putstr("Please input task name:\n\r");
        }
    }
    else if(func == '2'){
        bios_putstr("[Func2] Run task by bat1(id-order)\n\r");
        bios_putstr("\t bat info:");
        for(int i=0;i<bat_sz1-1;i++)
            bios_putchar(batbuf1[i]);
        bios_putstr("\n\r");
        for(int i=0;i<bat_id_ptr;i++){
        if(bat_id_array[i] < 0 || bat_id_array[i] >= tasknum)
            bios_putstr("[bat1-err]task id err\n\r");
        else
            load_task_img(bat_id_array[i]);
        }
    }
    else if(func == '3'){
        bios_putstr("[Func3] Run task by bat2(name-order)\n\r");
        bios_putstr("\t bat info:");
        for(int i=0;i<bat_sz1-1;i++)
            bios_putchar(batbuf2[i]);
        bios_putstr("\n\r");
        for(int i=0;i<bat_name_ptr;i++){
            taskid = -1;
            for(int j=0;j<tasknum;j++){
                if(strcmp(tasks[j].name,bat_name_array[i])==0){
                    taskid = j;
                }
            }
            if(taskid == -1)
                bios_putstr("[bat2-err]task name err\n\r");
            else
                load_task_img(taskid);
        }
    }

    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        asm volatile("wfi");
    }

    return 0;
}
