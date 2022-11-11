#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/sched.h>

// NOTE: A/C-core
static ptr_t kernMemCurr = FREEMEM_KERNEL;
ptr_t Page_Addr[MAXPAGE]={0};
char  Page_Flag[MAXPAGE]={0}; //0表示可用，1表示被占用

void init_mm(){
    //初始化可用的页表数组，默认一页
    for(int i=0;i<MAXPAGE;i++){
        Page_Addr[i] = kernMemCurr;
        Page_Flag[i] = 0; //空闲
        kernMemCurr += PAGE_SIZE;
    }
}

ptr_t allocPage(int numPage,pcb_t *pcbptr)
{
    // align PAGE_SIZE
    int flag=1;
    int hitnum;
    for(int i=0;i<MAXPAGE;i++){
        if(Page_Flag[i]==0){
            flag=0;
            hitnum=i;
            break;
        }
    }
    if(flag==1)
        return 0; //无可用页，返回非法地址
    else{
        Page_Flag[hitnum]=1; //标记占用
        pcbptr->pg_addr[pcbptr->pg_num++] = Page_Addr[hitnum];
        return Page_Addr[hitnum];
    }
}

void freePage(ptr_t baseAddr)
{
    // TODO [P4-task1] (design you 'freePage' here if you need):
    int page_id = (baseAddr - FREEMEM_KERNEL) / PAGE_SIZE;
    Page_Flag[page_id] = 0;
}

void *kmalloc(size_t size)
{
    // TODO [P4-task1] (design you 'kmalloc' here if you need):
}


/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO [P4-task1] share_pgtable:
    //将内核页表赋值到用户页表当中，从而用户进程进入内核时不会产生缺页
    //事实上就是将内核页表的后半部分（0xfff开头）填入了用户页表对应位置，该过程需要在clear之后，设置程序页表之前
    memcpy((uint8_t *)dest_pgdir,(uint8_t *)src_pgdir,PAGE_SIZE); 
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page
   */
//为虚拟地址va在三级页表的页表目录pgdir中进行相应映射，最终返回映射地址对应的内核虚地址
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir,pcb_t *pcbptr)
{
    // TODO [P4-task1] alloc_page_helper:
    va = va & VA_MASK;
    ptr_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    ptr_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    ptr_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> NORMAL_PAGE_SHIFT);
    //L2页表已有地址，此处为L1和L0页表地址和最终物理页的虚拟地址
    ptr_t pgdir1,pgdir0,pg_vpa;
    //对应提取出的物理地址
    ptr_t pgdir1_ppn,pgdir0_ppn,pg_pa;
    ptr_t valid2,valid1,valid0;
    //需要置起的flag位
    ptr_t bit_21 = _PAGE_PRESENT | _PAGE_USER ;
    ptr_t bit_0  = _PAGE_PRESENT |_PAGE_USER | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY;
    //L2 中的页表项
    ptr_t pgdir2_entry = ((PTE *)pgdir)[vpn2];
    valid2 = pgdir2_entry & _PAGE_PRESENT;
    if(valid2 == 0){
        pgdir1 = allocPage(1,pcbptr);
        pgdir1_ppn = (kva2pa(pgdir1)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir+vpn2,pgdir1_ppn | bit_21);
    }
    else{
        pgdir1 = pa2kva(get_pa(pgdir2_entry));
    }
    //L1 中的页表项
    ptr_t pgdir1_entry = ((PTE *)pgdir1)[vpn1];
    valid1 = pgdir1_entry & _PAGE_PRESENT;
    if(valid1 == 0){
        pgdir0 = allocPage(1,pcbptr);
        pgdir0_ppn = (kva2pa(pgdir0)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir1+vpn1,pgdir0_ppn | bit_21);
    }
    else{
        pgdir0 = pa2kva(get_pa(pgdir1_entry));
    }
    //L0 中的页表项
    ptr_t pgdir0_entry = ((PTE *)pgdir0)[vpn0];
    valid0 = pgdir0_entry & _PAGE_PRESENT;
    if(valid0 == 0){
        pg_vpa = allocPage(1,pcbptr);
        pg_pa = (kva2pa(pg_vpa)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir0+vpn0,pg_pa | bit_0);
    }
    else{
        pg_vpa = pa2kva(get_pa(pgdir0_entry));
    }
    return pg_vpa;
}


uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
}

void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
}
