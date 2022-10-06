#include <os/mm.h>

static ptr_t kernMemCurr = FREEMEM_KERNEL;
static ptr_t userMemCurr = FREEMEM_USER;

ptr_t KernelPage_Addr[MAXPAGE]={0};
char  KernelPage_Flag[MAXPAGE]={0}; //0表示可用，1表示被占用
ptr_t UserPage_Addr[MAXPAGE]={0};
char  UserPage_Flag[MAXPAGE]={0};

// ptr_t allocKernelPage(int numPage)
// {
//     // align PAGE_SIZE
//     ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
//     kernMemCurr = ret + numPage * PAGE_SIZE;
//     return ret;
// }

// ptr_t allocUserPage(int numPage)
// {
//     // align PAGE_SIZE
//     ptr_t ret = ROUND(userMemCurr, PAGE_SIZE);
//     userMemCurr = ret + numPage * PAGE_SIZE;
//     return ret;
// }

void init_mm(){
    //初始化可用的页表数组，默认一页
    for(int i=0;i<MAXPAGE;i++){
        KernelPage_Addr[i] = kernMemCurr;
        KernelPage_Flag[i] = 0; //空闲
        kernMemCurr += PAGE_SIZE;
    }
    for(int i=0;i<MAXPAGE;i++){
        UserPage_Addr[i] = userMemCurr;
        UserPage_Flag[i] = 0;
        userMemCurr += PAGE_SIZE;
    }
}

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