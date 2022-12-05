#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>
#include <assert.h>

#define TMPADDR 
// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;
// static uintptr_t io_base = 0xffffffc05a000000lu;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // TODO: [p5-task1] map one specific physical region to virtual address
    uintptr_t kernel_pgdir = PGDIR_PA + KVA_PA_OFFSET;
    uintptr_t kva = io_base; //返回起始地址
    uintptr_t va, vpn2, vpn1, vpn0;
    while(size > 0)
    {
        va = io_base & VA_MASK;
        vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
        vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
        vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> NORMAL_PAGE_SHIFT);
        //L2页表已有地址，此处为L1和L0页表地址和最终物理页的虚拟地址
        ptr_t pgdir1,pgdir0;
        int pgdir1_id,pgdir0_id; //对应页标号
        //对应提取出的物理地址
        ptr_t pgdir1_ppn,pgdir0_ppn,pfn;
        ptr_t valid2,valid1,valid0;
        //需要置起的flag位
        ptr_t bit_21 = _PAGE_PRESENT ;
        ptr_t bit_0  = _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY;
        //L2 中的页表项
        ptr_t pgdir2_entry = ((PTE *)kernel_pgdir)[vpn2];
        valid2 = pgdir2_entry & _PAGE_PRESENT;
        if(valid2 == 0){
            pgdir1_id = allocPage(1);
            ava_page[pgdir1_id].pid = 0;
            pgdir1 = ava_page[pgdir1_id].addr;
            pgdir1_ppn = (kva2pa(pgdir1)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
            set_attribute((PTE *)kernel_pgdir+vpn2,pgdir1_ppn | bit_21);
            clear_pgdir(pgdir1);
        }
        else{
            pgdir1 = pa2kva(get_pa(pgdir2_entry));
        }
        //L1 中的页表项
        ptr_t pgdir1_entry = ((PTE *)pgdir1)[vpn1];
        valid1 = pgdir1_entry & _PAGE_PRESENT;
        if(valid1 == 0){
            pgdir0_id = allocPage(1);
            ava_page[pgdir0_id].pid = 0;
            pgdir0 = ava_page[pgdir0_id].addr;
            pgdir0_ppn = (kva2pa(pgdir0)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
            set_attribute((PTE *)pgdir1+vpn1,pgdir0_ppn | bit_21);
            clear_pgdir(pgdir0);
        }
        else{
            pgdir0 = pa2kva(get_pa(pgdir1_entry));
        }
        //L0 中的页表项
        ptr_t pgdir0_entry = ((PTE *)pgdir0)[vpn0];
        valid0 = pgdir0_entry & _PAGE_PRESENT;
        if(valid0 == 0){
            pfn = (phys_addr>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
            set_attribute((PTE *)pgdir0+vpn0,pfn | bit_0);
        }
        else{
            assert(0);
        }
        io_base += PAGE_SIZE;
        phys_addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    flush_all();
    return kva;
}

void iounmap(void *io_addr)
{
    // TODO: [p5-task1] a very naive iounmap() is OK
    // maybe no one would call this function?
}
