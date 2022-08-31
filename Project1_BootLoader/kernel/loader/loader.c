#include <os/task.h>
#include <os/string.h>
#include <os/bios.h>
#include <type.h>

#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

uint64_t load_task_img(int taskid)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */

    //task3:
    // int task_addr = TASK_MEM_BASE+taskid*TASK_SIZE;
    // int block_num = 15; //fix in task3
    // int block_id = 1+(taskid+1)*block_num;
    // bios_sdread(task_addr,block_num,block_id);

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

    //call the task
    void (*func)(void) = (void *)task_mem_addr;
    func();
    return 0;
}