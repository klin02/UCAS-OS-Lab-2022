#include <os/mm.h>

// NOTE: A/C-core
static ptr_t kernMemCurr = FREEMEM_KERNEL;

ptr_t allocPage(int numPage)
{
    // align PAGE_SIZE
    ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
    kernMemCurr = ret + numPage * PAGE_SIZE;
    return ret;
}

// NOTE: Only need for S-core to alloc 2MB large page
#ifdef S_CORE
static ptr_t largePageMemCurr = LARGE_PAGE_FREEMEM;
ptr_t allocLargePage(int numPage)
{
    // align LARGE_PAGE_SIZE
    ptr_t ret = ROUND(largePageMemCurr, LARGE_PAGE_SIZE);
    largePageMemCurr = ret + numPage * LARGE_PAGE_SIZE;
    return ret;    
}
#endif

void freePage(ptr_t baseAddr)
{
    // TODO [P4-task1] (design you 'freePage' here if you need):
}

void *kmalloc(size_t size)
{
    // TODO [P4-task1] (design you 'kmalloc' here if you need):
}


/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO [P4-task1] share_pgtable:
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO [P4-task1] alloc_page_helper:
}

uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
}

void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
}

//tmp
static ptr_t userMemCurr = FREEMEM_USER;
#define MAXPAGE 32
ptr_t KernelPage_Addr[MAXPAGE]={0};
char  KernelPage_Flag[MAXPAGE]={0}; //0表示可用，1表示被占用
ptr_t UserPage_Addr[MAXPAGE]={0};
char  UserPage_Flag[MAXPAGE]={0};


ptr_t allocKernelPage(int numPage){//按照1准备
    int flag=1;
    int hitnum;
    for(int i=0;i<MAXPAGE;i++){
        if(KernelPage_Flag[i]==0){
            flag=0;
            hitnum=i;
            break;
        }
    }
    if(flag==1)
        return 0; //无可用页，返回非法地址
    else{
        KernelPage_Flag[hitnum]=1; //标记占用
        return KernelPage_Addr[hitnum];
    }
}

ptr_t allocUserPage(int numPage){//按照1准备
    int flag=1;
    int hitnum;
    for(int i=0;i<MAXPAGE;i++){
        if(UserPage_Flag[i]==0){
            flag=0;
            hitnum=i;
            break;
        }
    }
    if(flag==1)
        return 0; //无可用页，返回非法地址
    else{
        UserPage_Flag[hitnum]=1;//标记占用
        return UserPage_Addr[hitnum];
    }
}

void freeKernelPage(int page_num){
    //参数为需要回收的页表号
    KernelPage_Flag[page_num] = 0; //改标记即可，无需清空，程序内部保证了所用值被初始化
}

void freeUserPage(int page_num){
    //参数为需要回收的页表号
    UserPage_Flag[page_num] = 0; //改标记即可，无需清空，程序内部保证了所用值被初始化
}
