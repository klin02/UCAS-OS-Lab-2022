#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/smp.h>
#include <os/mm.h>
#include <pgtable.h>
#include <printk.h>
#include <assert.h>
#include <screen.h>

//不调用csr.h库，新增宏定义
#define SCAUSE_IRQ_FLAG   (1UL << 63)
//#define TIMER_INTERVAL (time_base/20000)
handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    // TODO: [p2-task3] & [p2-task4] interrupt handler.
    // call corresponding handler by the value of `scause`
    //相关定义见csr.h
    lock_kernel();
    current_running = (get_current_cpu_id() ==0) ? current_running_0 : current_running_1;           
    uint64_t type = scause & SCAUSE_IRQ_FLAG; //除最高位全零
    uint64_t code = scause & ~SCAUSE_IRQ_FLAG; //最高位为零
    if(type)
        irq_table[code](regs,stval,scause);
    else
        exc_table[code](regs,stval,scause);
}

void handle_irq_timer(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    // TODO: [p2-task4] clock interrupt handler.
    // Note: use bios_set_timer to reset the timer and remember to reschedule
    check_sleeping();
    set_timer(get_ticks()+TIMER_INTERVAL);
    do_scheduler();
}

void init_exception()
{
    /* TODO: [p2-task3] initialize exc_table */
    /* NOTE: handle_syscall, handle_other, etc.*/
    //handler_t 是指向void(...)函数的指针
    for(int i=0;i<EXCC_COUNT;i++)
        exc_table[i] = &handle_other;
    exc_table[EXCC_SYSCALL] = &handle_syscall;
    exc_table[EXCC_INST_PAGE_FAULT] = &handle_ld_pagefault;    //本次实验未测试该例外
    exc_table[EXCC_LOAD_PAGE_FAULT] = &handle_ld_pagefault;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_st_pagefault;
    /* TODO: [p2-task4] initialize irq_table */
    /* NOTE: handle_int, handle_other, etc.*/
    for(int i=0;i<IRQC_COUNT;i++)
        irq_table[i] = &handle_other;
    irq_table[IRQC_S_TIMER] = &handle_irq_timer;
    /* TODO: [p2-task3] set up the entrypoint of exceptions */
    //调用汇编函数setup_exception 完成相关操作
    setup_exception();
}

void handle_ld_pagefault(regs_context_t *regs, uint64_t stval, uint64_t scause){
    int res = bit_setter(stval,current_running->pgdir,0);
    if(res == 1) return ;
    else{
        handle_pagefault(stval);
        return ;
    }
}

void handle_st_pagefault(regs_context_t *regs, uint64_t stval, uint64_t scause){
    int res = bit_setter(stval,current_running->pgdir,1);
    if(res == 1) return ;
    else{
        handle_pagefault(stval);
        return ;
    }
}

void handle_pagefault(uint64_t stval){
    ptr_t align_vaddr = ((stval & VA_MASK) >> NORMAL_PAGE_SHIFT ) << NORMAL_PAGE_SHIFT;
    int swap_find = 0;
    int swap_id = -1;
    for(int i=0;i<SWAP_PAGE;i++){
        if(swap_page[i].valid == 0)
            continue;
        if(swap_page[i].pid == current_running->pid && swap_page[i].vaddr == align_vaddr)
        {
            swap_find = 1;
            swap_id = i;
            break;
        }
    }
    if(swap_find == 1)
    {
        //分配一页，从swap区调入并更新页节点相关信息
        int ava_id = allocPage(1);
        ptr_t ava_kva = ava_page[ava_id].addr;
        ptr_t ava_pa = kva2pa(ava_kva);
        bios_sdread(ava_pa,8,swap_page[swap_id].block_id);
        swap_page[swap_id].valid = 0;
        ava_page[ava_id].pid = swap_page[swap_id].pid;
        ava_page[ava_id].vaddr = swap_page[swap_id].vaddr;
        ava_page[ava_id].ppte = swap_page[swap_id].ppte;
        // 更新页表项信息
        // ptr_t bit_0  = _PAGE_PRESENT |_PAGE_USER | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY;
        ptr_t ava_ppn = (ava_pa >> NORMAL_PAGE_SHIFT) << _PAGE_PFN_SHIFT;
        set_attribute(ava_page[ava_id].ppte,ava_ppn | swap_page[swap_id].bit);
        // 增加入可换出队列
        port_page_list[port_list_head] = ava_id;
        port_list_head = (port_list_head + 1) % MAXPAGE;
    }
    else{
        alloc_page_helper(stval,current_running->pgdir,current_running);
    }
        
    local_flush_tlb_all();
    return;
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    printk("Core %d PID %d\n\r",get_current_cpu_id(),current_running->pid);
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lu\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    printk("tval: 0x%lx cause: 0x%lx\n", stval, scause);
    assert(0);
}
