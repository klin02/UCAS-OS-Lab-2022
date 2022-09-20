#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <type.h>

#define SECTOR_SIZE 512

uint64_t load_task_img(int taskid)
{//P2更改：不跳转，只加载，依赖库由bios改为kernel
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */

    //task4:
    //load task-covered sector to memory
    int task_mem_addr = TASK_MEM_BASE + taskid*TASK_SIZE;
    int task_block_id = tasks[taskid].entry / SECTOR_SIZE;
    int task_tail_id = (tasks[taskid].entry + tasks[taskid].size) / SECTOR_SIZE;
    int task_block_num = task_tail_id - task_block_id +1;
    int task_entry_offset = tasks[taskid].entry - task_block_id*SECTOR_SIZE;
    bios_sdread(task_mem_addr,task_block_num,task_block_id);

    //move task ahead to be aligned
    unsigned char * dest = (unsigned int *)task_mem_addr;
    unsigned char * src = dest + task_entry_offset;
    memcpy(dest,src,tasks[taskid].size);

    //clear tail
    dest += tasks[taskid].size;
    bzero(dest,task_entry_offset);

    // //call the task
    // void (*func)(void) = (void *)task_mem_addr;
    // func();
    //asm volatile ("jalr %0" : : "r"(task_mem_addr));
    return 0;
}