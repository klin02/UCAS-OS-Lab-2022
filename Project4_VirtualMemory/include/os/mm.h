/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */
#ifndef MM_H
#define MM_H

#include <type.h>
#include <pgtable.h>
#include <os/sched.h>

#define MAP_KERNEL 1
#define MAP_USER 2
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define INIT_KERNEL_STACK_0 0xffffffc052000000
#define INIT_KERNEL_STACK_1 (INIT_KERNEL_STACK_0+PAGE_SIZE)
#define FREEMEM_KERNEL 0xffffffc052003000
/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

typedef struct {
        ptr_t addr;   //kva
        char  valid;
        // char  en_swap;  //标志能否换出
        int   pid;
        ptr_t vaddr;  //页对齐后，取掩码后的虚地址
        PTE * ppte;   //映射到该页的页表项
}Page_Node;

typedef struct {
        int   block_id;
        char  valid;
        // char  en_swap;  //标志能否换出
        int   pid;
        ptr_t vaddr;  //页对齐后，取掩码后的虚地址
        PTE * ppte;   //映射到该页的页表项
}Swap_Node;

//mode：0 fix 1 port 可换出
extern int allocPage(int numPage);
// TODO [P4-task1] */
extern void freePage(ptr_t baseAddr);

// #define S_CORE
// NOTE: only need for S-core to alloc 2MB large page
#ifdef S_CORE
#define LARGE_PAGE_FREEMEM 0xffffffc056000000
#define USER_STACK_ADDR 0x400000
extern ptr_t allocLargePage(int numPage);
#else
// NOTE: A/C-core
#define USER_STACK_ADDR 0xf00010000
#endif

// TODO [P4-task1] */
extern void* kmalloc(size_t size);
extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir,pcb_t *pcbptr);

// TODO [P4-task4]: shm_page_get/dt */
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);

#define MAXPAGE 512  //0x100
// extern ptr_t Page_Addr[MAXPAGE];
// extern char  Page_Flag[MAXPAGE]; //0表示可用，1表示被占用

extern Page_Node ava_page[MAXPAGE];

extern int port_page_list[MAXPAGE]; //可换出页的序号列表，FIFO逻辑管理
extern int port_list_head;
extern int port_list_tail;

#define SWAP_PAGE 4096

extern Swap_Node swap_page[SWAP_PAGE];

#define SECTOR_SIZE 512
extern int swap_block_offset;
extern void init_mm();
#endif /* MM_H */
