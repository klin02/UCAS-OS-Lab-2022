#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/mm.h>
#include <type.h>


uint64_t load_task_img(int taskid,ptr_t pgdir,pcb_t *pcbptr)
{//P2更改：不跳转，只加载，依赖库由bios改为kernel
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */

    //prj4:
    int task_block_id = tasks[taskid].entry / SECTOR_SIZE;
    int task_tail_id = (tasks[taskid].entry + tasks[taskid].size) / SECTOR_SIZE;
    int task_block_num = task_tail_id - task_block_id +1;
    int offset = tasks[taskid].entry - task_block_id*SECTOR_SIZE;
    int memsz = tasks[taskid].memsz;
    int cur_block = task_block_id; //当前欲加载的起始扇区号
    int rest_block = task_block_num; //剩余未加载的扇区数
    ptr_t cur_va = tasks[taskid].vaddr; //当前要加载到的用户虚拟地址
    ptr_t kva[16]; //映射到的内核虚地址,为了实现跨页对齐，都需要保存
    for(int i=0;i<16;i++)
        kva[i]=0;
    int kva_ptr = 0;
    //由于要求4KB页，一次只能加载8个扇区，地址间隔为0x1000
    while(rest_block > 8){
        kva[kva_ptr] = alloc_page_helper(cur_va,pgdir,pcbptr);
        bios_sdread(kva2pa(kva[kva_ptr]),8,cur_block);
        cur_block += 8;
        rest_block -=8;
        cur_va += PAGE_SIZE;
        kva_ptr++;
    }
    if(rest_block!=0){ //不是正好整页，额外操作一次
        kva[kva_ptr] = alloc_page_helper(cur_va,pgdir,pcbptr);
        bios_sdread(kva2pa(kva[kva_ptr]),rest_block,cur_block);
        cur_va += PAGE_SIZE;
        kva_ptr++;
    }
    //至此，filesz大小的未对齐程序加载到了内存中

    //跨页的前移对齐
    for(int i=0;i<kva_ptr-1;i++)
    {
        memcpy((unsigned char *)kva[i],(unsigned char *)(kva[i]+offset),PAGE_SIZE-offset);
        memcpy((unsigned char *)(kva[i]+PAGE_SIZE-offset),(unsigned char *)kva[i+1],offset);
    }
    memcpy((unsigned char *)kva[kva_ptr-1],(unsigned char *)(kva[kva_ptr-1]+offset),PAGE_SIZE-offset);
    //清空fileze之后的部分
    //尾部相对于页头的偏移，如果头部偏移较大，可能为负
    int tail_offset = tasks[taskid].size - (kva_ptr-1)*PAGE_SIZE;
    if(tail_offset < 0){
        //清空倒数第二页尾端
        tail_offset = 0 - tail_offset;
        bzero((unsigned char *)(kva[kva_ptr-2]+PAGE_SIZE-tail_offset),tail_offset);
    }
    else{
        bzero((unsigned char *)(kva[kva_ptr-1]+tail_offset),PAGE_SIZE-tail_offset);
    }

    //考虑memsz所需的页数大于当前页数，额外分配
    int mem_pg = memsz / PAGE_SIZE + (memsz % PAGE_SIZE !=0);
    if(mem_pg > kva_ptr)//所需分配的页大于当前分配的页
    {
        int rest_pg = mem_pg - kva_ptr;
        while(rest_pg > 0)
        {
            kva[kva_ptr++] = alloc_page_helper(cur_va,pgdir,pcbptr);
            bzero((unsigned char *)kva[kva_ptr-1],PAGE_SIZE); //新增页均为空
            rest_pg -- ;
            cur_va += PAGE_SIZE;
        }
    }
    // 不再直接跳转到用户程序
    return 0;
}