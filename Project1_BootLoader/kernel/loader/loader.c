#include <os/task.h>
#include <os/string.h>
#include <os/bios.h>
#include <type.h>

uint64_t load_task_img(int taskid)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */

    int task_addr = TASK_MEM_BASE+taskid*TASK_SIZE;
    int block_num = 15; //fix in task3
    int block_id = 1+(taskid+1)*block_num;
    bios_sdread(task_addr,block_num,block_id);

    void (*func)(void) = (void *)task_addr;
    func();
    return 0;
}