#include <common.h>
#include <asm.h>
#include <os/bios.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <type.h>

#define VERSION_BUF 50

//16个任务地址为0x52000000~0x520fffff
#define USER_INFO_ADDR 0x52200000
//BOOTLOADER_ENTRYPOINT 0x50200000   BOOT_LOADER_SIG_OFFSET 0x1fe
int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

// Task info array
task_info_t tasks[TASK_MAXNUM];
short tasknum;

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

int main(void)
{
    // Check whether .bss section is set to zero
    int check = bss_check();

    // Init jump table provided by BIOS (ΦωΦ)
    init_bios();

    // Init task information (〃'▽'〃)
    init_task_info();


    // Output 'Hello OS!', bss check result and OS version
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int i, output_val_pos = 0;

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    
    for (i = 0; i < sizeof(output_str); ++i)
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
    int taskid=0;
    bios_putstr("Please input task id (end with a non-num char):\n\r");
    int screen_ch;
    while(1){
        while((screen_ch = bios_getchar())==-1);    //loop until getting a char
        bios_putchar(screen_ch);
        if(screen_ch < '0' || screen_ch > '9')
            break;
        else
            taskid = taskid*10 + screen_ch - '0';
    }
    load_task_img(taskid);

    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        asm volatile("wfi");
    }

    return 0;
}
