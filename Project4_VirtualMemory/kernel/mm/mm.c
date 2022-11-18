#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/kernel.h>
#include <assert.h>

// NOTE: A/C-core
static ptr_t kernMemCurr = FREEMEM_KERNEL;
// ptr_t Page_Addr[MAXPAGE]={0};
// char  Page_Flag[MAXPAGE]={0}; //0表示可用，1表示被占用

Page_Node ava_page[MAXPAGE];
int port_page_list[MAXPAGE]; //可换出页的序号列表，FIFO逻辑管理
//仿照fifo循环队列的数组管理,head tail初始相等
//head和tail相同为空，当head+1与tail取模相同时表示满（事实上无法达到，因为port页数小于物理页数）
int port_list_head;
int port_list_tail;
Swap_Node swap_page[SWAP_PAGE];

Shm_Index shm_page_index[SHM_MAX_PAGE];

void init_mm(){
    //初始化可用的页表数组，默认一页
    for(int i=0;i<MAXPAGE;i++){
        ava_page[i].addr = kernMemCurr;
        ava_page[i].valid = 0;//空闲
        kernMemCurr += PAGE_SIZE;
    }
    for(int i=0;i<MAXPAGE;i++){
        port_page_list[MAXPAGE] = 0;
    }
    port_list_head = port_list_tail = 0;
    for(int i=0;i<SWAP_PAGE;i++){
        swap_page[i].block_id = swap_block_offset + i*8; //每个swap对应的起始扇区号
        swap_page[i].valid = 0;
    }
    for(int i=0;i<SHM_MAX_PAGE;i++){
        shm_page_index[i].id = 0;
        shm_page_index[i].valid = 0;
        shm_page_index[i].visitor = 0;
    }
}

//更改：为便于页管理，返回分配页的id。进入portlist也在外部完成
int allocPage(int numPage)
{
    // align PAGE_SIZE
    int avafull=1;
    int hitnum;
    for(int i=0;i<MAXPAGE;i++){
        if(ava_page[i].valid==0){
            avafull=0;
            hitnum=i;
            break;
        }
    }
    if(avafull==1)//可用页已满
    {
        printk("Trigger swap!\n");
        while(1);
        if(port_list_head == port_list_tail) //无可换出页
        {
            printk("Port list empty!\n");
            assert(0);
        }    
        else{
            int swapfull = 1;
            int swap_id = -1;
            for(int i=0;i<SWAP_PAGE;i++)
                if(swap_page[i].valid==0){
                    swapfull = 0;
                    swap_id = i;
                    break;
                }
            if(swapfull == 1) //swap区已满
            {
                printk("Swap Full!\n");
                assert(0);
            }    
            else //有可换出页，且swap区未满
            {
                //拷贝该页到swap取，并清空映射该换出页的表项
                int port_id = port_page_list[port_list_tail];
                port_list_tail = (port_list_tail + 1)%MAXPAGE;
                ptr_t port_pa = get_pa(*(ava_page[port_id].ppte));
                ptr_t port_bit = get_attribute(*(ava_page[port_id].ppte),_PAGE_PFN_MASK);
                *(ava_page[port_id].ppte) = 0;
                bios_sdwrite(port_pa,8,swap_page[swap_id].block_id);
                //设置swap节点
                swap_page[swap_id].valid = 1;
                swap_page[swap_id].pid = ava_page[port_id].pid;
                swap_page[swap_id].vaddr = ava_page[port_id].vaddr;
                swap_page[swap_id].ppte = ava_page[port_id].ppte;
                swap_page[swap_id].bit = port_bit;
                //将原先的物理页分配出去
                ava_page[port_id].pid = 0;
                ava_page[port_id].vaddr = 0;
                ava_page[port_id].ppte = 0;
                bzero((void *)ava_page[port_id].addr,PAGE_SIZE);
                return port_id;
            }
        }
    }
    else{
        ava_page[hitnum].valid=1;
        bzero((void *)ava_page[hitnum].addr,PAGE_SIZE);
        return hitnum;
    }
}

void freePage(ptr_t baseAddr)
{
    // TODO [P4-task1] (design you 'freePage' here if you need):
    // int page_id = (baseAddr - FREEMEM_KERNEL) / PAGE_SIZE;
    // Page_Flag[page_id] = 0;
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
    int pgdir1_id,pgdir0_id,pg_vpa_id; //对应页标号
    //对应提取出的物理地址
    ptr_t pgdir1_ppn,pgdir0_ppn,pg_pa;
    ptr_t valid2,valid1,valid0;
    //需要置起的flag位
    ptr_t bit_21 = _PAGE_PRESENT | _PAGE_USER ;
    ptr_t bit_0  = _PAGE_PRESENT |_PAGE_USER | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC;
    //L2 中的页表项
    ptr_t pgdir2_entry = ((PTE *)pgdir)[vpn2];
    valid2 = pgdir2_entry & _PAGE_PRESENT;
    if(valid2 == 0){
        pgdir1_id = allocPage(1);
        ava_page[pgdir1_id].pid = pcbptr->pid;
        pgdir1 = ava_page[pgdir1_id].addr;
        pgdir1_ppn = (kva2pa(pgdir1)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir+vpn2,pgdir1_ppn | bit_21);
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
        ava_page[pgdir0_id].pid = pcbptr->pid;
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
        pg_vpa_id = allocPage(1);
        pg_vpa = ava_page[pg_vpa_id].addr;
        pg_pa = (kva2pa(pg_vpa)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir0+vpn0,pg_pa | bit_0);
        clear_pgdir(pg_vpa);
        //设置可用页节点内容
        ptr_t align_vaddr = (va >> NORMAL_PAGE_SHIFT) << NORMAL_PAGE_SHIFT;
        ava_page[pg_vpa_id].pid = pcbptr->pid;
        ava_page[pg_vpa_id].vaddr = align_vaddr;
        ava_page[pg_vpa_id].ppte = (PTE *)pgdir0+vpn0;
        //最后一级物理页，加入可替换队列
        if((port_list_head+1) % MAXPAGE != port_list_tail % MAXPAGE) //非满
        {
            port_page_list[port_list_head] = pg_vpa_id;
            port_list_head = (port_list_head + 1) % MAXPAGE;
        }        
        else{
            printk("port list full!\n");
            assert(0);
        }
    }
    else{
        pg_vpa = pa2kva(get_pa(pgdir0_entry));
    }
    return pg_vpa;
}

//功能：查找，如果发现表项存在，只是没有置位，则置位并返回1；否则返回0
 //mode: 0 ld 1 st 
int bit_setter(uintptr_t va, uintptr_t pgdir,int mode)
{
    va = va & VA_MASK;
    ptr_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    ptr_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    ptr_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> NORMAL_PAGE_SHIFT);
    ptr_t pgdir1,pgdir0,pg_vpa;
    ptr_t valid2,valid1,valid0;

    ptr_t bit_ld = _PAGE_ACCESSED;
    ptr_t bit_st = _PAGE_ACCESSED | _PAGE_DIRTY;
    ptr_t pgdir2_entry = ((PTE *)pgdir)[vpn2];
    valid2 = pgdir2_entry & _PAGE_PRESENT;
    if(valid2 == 0) return 0;
    else pgdir1 = pa2kva(get_pa(pgdir2_entry));

    ptr_t pgdir1_entry = ((PTE *)pgdir1)[vpn1];
    valid1 = pgdir1_entry & _PAGE_PRESENT;
    if(valid1 == 0) return 0;
    else pgdir0 = pa2kva(get_pa(pgdir1_entry));

    ptr_t pgdir0_entry = ((PTE *)pgdir0)[vpn0];
    valid0 = pgdir0_entry & _PAGE_PRESENT;
    if(valid0 == 0) return 0;
    else{
        if(mode == 0) set_attribute((PTE *)pgdir0+vpn0,bit_ld);
        else          set_attribute((PTE *)pgdir0+vpn0,bit_st);
        return 1;
    }
}

//功能：查找页表，如果发现虚地址存在，则直接返回0，（过程中不会分配页表）;否则分配中间页表，最后一级设置为传入pa，返回1
int pa_setter(uintptr_t pa,uintptr_t va,uintptr_t pgdir){
    va = va & VA_MASK;
    ptr_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    ptr_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    ptr_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> NORMAL_PAGE_SHIFT);

    ptr_t pgdir1,pgdir0,pg_vpa;
    int pgdir1_id,pgdir0_id,pg_vpa_id; //对应页标号
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
        pgdir1_id = allocPage(1);
        ava_page[pgdir1_id].pid = current_running->pid;
        pgdir1 = ava_page[pgdir1_id].addr;
        pgdir1_ppn = (kva2pa(pgdir1)>>NORMAL_PAGE_SHIFT)<<_PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir+vpn2,pgdir1_ppn | bit_21);
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
        pgdir0 = ava_page[pgdir0_id].addr;
        ava_page[pgdir0_id].pid = current_running->pid;
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
        //该虚地址不存在，可以将pa映射到该虚地址。在该函数前已经搜索过swap区，可以保证确实未分配
        //注意：pa应当处理为表项要求
        pa = (pa >> NORMAL_PAGE_SHIFT) << _PAGE_PFN_SHIFT;
        set_attribute((PTE *)pgdir0+vpn0,pa | bit_0);
        //共享页表不可加入可换出队列
        return 1;
    }
    else{
        return 0;
    }
}
uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
    int id = key % SHM_MAX_PAGE;
    ptr_t kva,pa;
    int pg_id;
    if(shm_page_index[id].valid == 0) //未被访问过
    {
        //分配并记录该共享页相应的物理地址，并以页结构id索引
        pg_id = allocPage(1);
        ava_page[pg_id].pid = 0;
        kva = ava_page[pg_id].addr;
        clear_pgdir(kva);
        shm_page_index[id].id = pg_id;
        shm_page_index[id].valid = 1;
        shm_page_index[id].visitor ++ ;
        pa = kva2pa(kva);
    }
    else{
        pg_id = shm_page_index[id].id;
        shm_page_index[id].visitor ++ ;
        kva = ava_page[pg_id].addr;
        pa = kva2pa(kva);
    }
    //返回对用户可见的虚地址，后续可根据该虚地址访问该位置
    //从基址开始搜索未被访问的地址
    ptr_t uva = SHM_PAGE_BASE;
    //搜索swap区，确认该进程未使用过该虚地址
    while(1){
        int swap_used = 0;
        for(int i=0;i<SWAP_PAGE;i++)
            if(swap_page[i].valid == 1 && swap_page[i].pid == current_running->pid && swap_page[i].vaddr == uva)
            {
                swap_used = 1;
                break;
            }
        if(swap_used == 1)
            uva += PAGE_SIZE;
        else
            break;
    }
    //搜索页表，如果虚地址已被分配，则返回0；否则将pa映射到该虚地址，并返回1
    while(1){
        int res = pa_setter(pa,uva,current_running->pgdir);
        if(res == 1)
            break;
        else
            uva += PAGE_SIZE;
    }
    return uva;
}

//功能：根据虚地址查询页表，如果查到有效映射，则清空，并返回物理页id。若错误，则返回-1
int shm_pgid(uintptr_t va,uintptr_t pgdir){
    va = va & VA_MASK;
    ptr_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    ptr_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    ptr_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> NORMAL_PAGE_SHIFT);
    ptr_t pgdir1,pgdir0,pg_vpa;
    ptr_t valid2,valid1,valid0;

    ptr_t pgdir2_entry = ((PTE *)pgdir)[vpn2];
    valid2 = pgdir2_entry & _PAGE_PRESENT;
    if(valid2 == 0) return -1; //未查找到有效映射，返回非法值-1。下同
    else pgdir1 = pa2kva(get_pa(pgdir2_entry));

    ptr_t pgdir1_entry = ((PTE *)pgdir1)[vpn1];
    valid1 = pgdir1_entry & _PAGE_PRESENT;
    if(valid1 == 0) return -1;
    else pgdir0 = pa2kva(get_pa(pgdir1_entry));

    ptr_t pgdir0_entry = ((PTE *)pgdir0)[vpn0];
    valid0 = pgdir0_entry & _PAGE_PRESENT;
    if(valid0 == 0) return -1;
    else{
        //提取内核虚地址，并清空表项
        pg_vpa = pa2kva(get_pa(pgdir0_entry));
        ((PTE *)pgdir0)[vpn0] = 0;
        //返回对应物理页地址
        int find_id = -1;
        for(int i=0;i<MAXPAGE;i++)
            if(ava_page[i].valid == 1 && ava_page[i].addr == pg_vpa)
            {
                find_id = i;
                break;
            }
        return find_id;
    }
    local_flush_tlb_all();
}
void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
    int ava_id = shm_pgid(addr,current_running->pgdir);
    if(ava_id == -1) //未找到该映射
        return ;
    int shm_id = -1;
    for(int i=0;i<SHM_MAX_PAGE;i++){
        if(shm_page_index[i].valid ==1 && shm_page_index[i].id == ava_id)
        {
            shm_id = i;
            break;
        }
    }
    if(shm_id == -1)
        return ; //非法
    else
    {
        shm_page_index[shm_id].visitor --;
        if(shm_page_index[shm_id].visitor == 0)
        {
            shm_page_index[shm_id].valid = 0;
            int abort_id = shm_page_index[shm_id].id;
            ava_page[abort_id].valid = 0;
            ava_page[abort_id].vaddr = 0;
            ava_page[abort_id].pid   = 0;
            ava_page[abort_id].ppte  = 0;
            //清空由分配时按需负责
        }
    }
    local_flush_tlb_all();
}
