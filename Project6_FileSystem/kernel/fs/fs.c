#include <os/string.h>
#include <os/fs.h>
#include <os/time.h>
#include <type.h>
#include <assert.h>
#include <os/mm.h>
#include <os/kernel.h>
#include <pgtable.h>

superblock_t superblock;
fdesc_t fdesc_array[NUM_FDESCS];
inode_t cur_dir_inode;
char block_map_buf[BLOCK_MAP_BYTE_SZ];
char inode_map_buf[INODE_MAP_BYTE_SZ];
char cur_dir_block[BLOCK_SIZE];
char tmp_sec[SECTOR_SIZE];
char tmp_blk[BLOCK_SIZE];
inode_t tmp_inode;

/************** 重要结构读写函数 *****************/
//注意一次最多读取64个扇区，即32K，在大于8块处再行考虑
void read_superblock(){  bios_sdread(kva2pa((uintptr_t)&superblock),1,SUPER_BLOCK_SEC_START); }
void write_superblock(){bios_sdwrite(kva2pa((uintptr_t)&superblock),1,SUPER_BLOCK_SEC_START); }

void read_block_map(){   bios_sdread(kva2pa((uintptr_t)block_map_buf),superblock.block_map_sec,superblock.block_map_start);}
void write_block_map(){ bios_sdwrite(kva2pa((uintptr_t)block_map_buf),superblock.block_map_sec,superblock.block_map_start);}

void read_inode_map(){   bios_sdread(kva2pa((uintptr_t)inode_map_buf),superblock.inode_map_sec,superblock.inode_map_start);}
void write_inode_map(){ bios_sdwrite(kva2pa((uintptr_t)inode_map_buf),superblock.inode_map_sec,superblock.inode_map_start);}

void read_inode(inode_t *dest,unsigned int inode_id){
    //考虑到1个扇区只有4个inode，首先按4个一组计算base。
    unsigned int base = inode_id / (SECTOR_SIZE / superblock.inode_size);
    unsigned int offset = inode_id % (SECTOR_SIZE / superblock.inode_size);
    unsigned int sec_id = superblock.inode_start + base;
    bios_sdread(kva2pa((uintptr_t)tmp_sec),1,sec_id);
    memcpy((char *)dest,tmp_sec + offset * superblock.inode_size,superblock.inode_size);
}
void write_inode(inode_t *src,unsigned int inode_id){
    unsigned int base = inode_id / (SECTOR_SIZE / superblock.inode_size);
    unsigned int offset = inode_id % (SECTOR_SIZE / superblock.inode_size);
    unsigned int sec_id = superblock.inode_start + base;
    bios_sdread(kva2pa((uintptr_t)tmp_sec),1,sec_id);
    memcpy(tmp_sec + offset * superblock.inode_size,(char *)src,superblock.inode_size);
    bios_sdwrite(kva2pa((uintptr_t)tmp_sec),1,sec_id);
}

void read_block(char *dest,unsigned int block_id){
    unsigned int sec_id = superblock.block_start + (block_id << 3);
    bios_sdread(kva2pa((uintptr_t)dest),8,sec_id);
}
void write_block(char *src,unsigned int block_id){
    unsigned int sec_id = superblock.block_start + (block_id << 3);
    bios_sdwrite(kva2pa((uintptr_t)src),8,sec_id);
}
/******************************************/

/****************分配函数*******************/
unsigned int alloc_inode(){
    if(superblock.inode_free == 0 )
        return -1;
    read_inode_map();
    unsigned int base; //8个一组(byte)，可用inode所在组
    unsigned int offset; //组内序号
    char mask;
    int found = 0;
    for(base = 0;base<INODE_MAP_BYTE_SZ;base++)
        if(inode_map_buf[base] != 0xff)
        {
            found = 1;
            break;
        }
    if(found == 0 ){
        assert(0);
    }
    else{
        for(offset=0;offset<8;offset++)
        {
            mask = 1<<offset;
            if((inode_map_buf[base]&mask) == 0){
                inode_map_buf[base] |= mask;
                break;
            }
            // else    
            //     printk("%d %d %d\n",inode_map_buf[base],mask,inode_map_buf[base]&mask);
        }
        assert(offset != 8);
        write_inode_map();
        superblock.inode_free -- ;
        write_superblock();
        //是否需要清空数据块并刷cache？
        //无需，所有数据都会在使用时指定
        return (base << 3) + offset;
    }
}
unsigned int alloc_block(){
    if(superblock.block_free == 0)
        return -1;
    read_block_map();
    unsigned int base; //8个一组(byte)
    unsigned int offset; //组内序号
    char mask;
    int found = 0;
    for(base = 0;base<BLOCK_MAP_BYTE_SZ;base++)
        if(block_map_buf[base] != 0xff)
        {
            found = 1;
            break;
        }
    if(found == 0 ){
        assert(0);
    }
    else{
        for(offset=0;offset<8;offset++)
        {
            mask = 1<<offset;
            if((block_map_buf[base]&mask) == 0){
                block_map_buf[base] |= mask;
                break;
            }
        }
        assert(offset != 8);
        assert((base<<3)+offset < superblock.block_num);
        write_block_map();
        superblock.block_free -- ;
        write_superblock();
        //是否需要清空数据块并刷cache？
        //无需，用作目录时，将进行初始化
        return (base << 3) + offset;
    }
}
void free_inode(unsigned int inode_id){ //包含释放该inode所使用的数据块
    unsigned int base = inode_id >> 3;
    unsigned int offset = inode_id % 8;
    char mask;
    mask = 1<<offset;
    assert((inode_map_buf[base]&mask) != 0);
    //目录固定释放direct0，文件需查找
    read_inode(&tmp_inode,inode_id);
    if(tmp_inode.type == I_DIR)
        free_block(tmp_inode.direct[0]);
    else{
        unsigned int blk = tmp_inode.file_size / BLOCK_SIZE + (tmp_inode.file_size % BLOCK_SIZE != 0);
        for(int i=0;i<blk;i++)
            block_index(i,&tmp_inode,2); //free block        
    }

    inode_map_buf[base] &= ~mask;
    write_inode_map();
    superblock.inode_free++;
    write_superblock();
}
void free_block(unsigned int block_id){
    unsigned int base = block_id >> 3;
    unsigned int offset = block_id % 8;
    char mask;
    mask = 1<<offset;
    // assert(block_map_buf[base]&mask !=0)
    if((block_map_buf[base]&mask) == 0)
        return ;
    block_map_buf[base] &= ~mask;
    write_block_map();
    superblock.block_free ++;
    write_superblock();
}
/*********************************************/

/******整合相关信息初始化inode及目录数据块*******/
void init_inode(inode_t* inode_ptr,unsigned int inode_id,unsigned int block_id,inode_type_t type,file_access_t access)
{
    inode_ptr->inode_id = inode_id;
    inode_ptr->owner_pid = current_running->pid;
    inode_ptr->type = type;
    inode_ptr->access = access;
    inode_ptr->link_cnt = 1;
    inode_ptr->file_size = 0;
    inode_ptr->create_time = get_timer();
    inode_ptr->modify_time = get_timer();
    inode_ptr->direct[0] = block_id;
    for(int i=1;i<DIRECT_NUM;i++)
        inode_ptr->direct[i] = 0;
    for(int i=0;i<INDIRECT_NUM_1;i++)
        inode_ptr->indirect1[i];
    for(int i=0;i<INDIRECT_NUM_2;i++)
        inode_ptr->indirect2[i];
    for(int i=0;i<INDIRECT_NUM_3;i++)
        inode_ptr->indirect3[i];
    write_inode(inode_ptr,inode_id);
}
//目录数据块
void init_dir_block(dentry_t *dir,unsigned int self_id,unsigned int parent_id,unsigned int self_blk_id)
{
    bzero(dir,BLOCK_SIZE);
    dir[0].inode_id = self_id;
    dir[0].dtype = D_DIR;
    dir[0].name[0] = '.';
    dir[0].name[1] = '\0';

    dir[1].inode_id = parent_id;
    dir[1].dtype = D_DIR;
    dir[1].name[0] = '.';
    dir[1].name[1] = '.';
    dir[1].name[2] = '\0';
    write_block((char *)dir,self_blk_id);
}
/*********************************************/

/*****************搜索函数********************/
int search_dentry(dentry_t *dir,char *path,dentry_type_t dtype){
    //根据名字及类型在目录中搜索
    //同名同类型返回index，同名不同类型返回-1，未找到返回-2
    int dentry_num = BLOCK_SIZE / superblock.dentry_size;
    for(int i=0;i<dentry_num;i++){
        if(dir[i].dtype == D_NULL)
            continue;
        if(strcmp(dir[i].name,path) == 0)
        {
            if(dir[i].dtype == dtype)
                return i;
            else
                return -1;
        }
    }
    return -2;
}
/********************************************/

/**************多级目录路径处理*****************/
int split_path(char* path,char dir[DIR_LEVEL][NAME_LEN]){
    if(path == NULL)
        return 0;
    int level = 0;
    int ptr = 0;
    for(int i=0;i<level;i++)
            bzero(dir[i],20);
    for(int i=0;i<strlen(path);i++){
            if(path[i] == '/'){
                    dir[level][ptr] = '\0';
                    ptr = 0;
                    level++;
            }
            else
                    dir[level][ptr++] = path[i];
    }
    dir[level][ptr] = '\0';
    level++;
    return level;
}
/*********************************************/

/*****************扩充文件大小******************/
//功能：根据所述文件中的顺序块号，按照索引路径查找。
//mode：0 拓展时使用，将补全索引路径，更改文件大小 
//      1 查找时使用，不改变索引路径和大小
//      2 释放时使用，将路径上的块全部释放（只取消标记，不影响索引过程）
//返回值：最终索引到的全局块号
unsigned int block_index(unsigned int file_blk,inode_t *inode,int mode){
    int add_blk = 0;
    int change_inode = 0;
    //不同层次块号上限(不计之前层次)
    unsigned int blk_upper[4]={
        DIRECT_NUM,
        INDIRECT_NUM_1 * INDIRECT1_INDEX,
        INDIRECT_NUM_2 * INDIRECT2_INDEX,
        INDIRECT_NUM_3 * INDIRECT3_INDEX                             
    };
    unsigned int blk_gap[4]={1,INDIRECT1_INDEX,INDIRECT2_INDEX,INDIRECT3_INDEX};
    //去除之前层次的块序号
    unsigned int cur_level_blk = file_blk;
    int level = -1;
    for(int i=0;i<4;i++){
        if(cur_level_blk < blk_upper[i]){
            level = i;
            break;
        }
        else{
            cur_level_blk -= blk_upper[i];
        }
    }
    assert(level >= 0);

    unsigned int base = cur_level_blk / blk_gap[level];
    unsigned int cur_index_blk;
    //block 0为根目录块，此处0表示未占用
    if(level == 0){
        if(inode->direct[base] == 0){
            assert(mode == 0);
            cur_index_blk = alloc_block();
            inode->direct[base] = cur_index_blk;
            change_inode = 1;
        }
        else
            cur_index_blk = inode->direct[base];
    }
    else if(level == 1){
        if(inode->indirect1[base] == 0){
            assert(mode == 0);
            cur_index_blk = alloc_block();
            inode->indirect1[base] = cur_index_blk;
            change_inode = 1;
        }
        else    
            cur_index_blk = inode->indirect1[base];
    }
    else if(level == 2){
        if(inode->indirect2[base] == 0){
            assert(mode == 0);
            cur_index_blk = alloc_block();
            inode->indirect2[base] = cur_index_blk;
            change_inode = 1;
        }
        else    
            cur_index_blk = inode->indirect2[base];
    }
    else if(level == 3){
        if(inode->indirect3[base] == 0){
            assert(mode == 0);
            cur_index_blk = alloc_block();
            inode->indirect3[base] = cur_index_blk;
            change_inode = 1;
        }
        else    
            cur_index_blk = inode->indirect3[base];
    }
    int clear; //新增块是否清空
    if(change_inode == 1){//新增块清空
        clear = 1;
        add_blk ++;
        write_inode(inode,inode->inode_id);
    }
    else{
        clear = 0;
        if(mode == 2){
            free_block(cur_index_blk);
            // printk("free! %d\n",cur_index_blk);
        }
            
    }
        
    unsigned int *index_table;
    unsigned int index_offset;
    unsigned int next_index_blk;
    for(int i=1;i<=level;i++){
        if(clear == 1)
            bzero(tmp_blk,BLOCK_SIZE);
        else
            read_block(tmp_blk,cur_index_blk);
        index_table = (unsigned int *)tmp_blk;
        index_offset = cur_level_blk % blk_gap[i];
        if(index_table[index_offset] == 0){ //空块的写回在此完成
            assert(mode == 0);
            next_index_blk = alloc_block();
            index_table[index_offset] = next_index_blk;
            write_block(tmp_blk,cur_index_blk);
            cur_index_blk = next_index_blk;
            clear = 1;
            add_blk ++;
        }
        else{//此处应当是读已有块，且无更改，不必写回
            cur_index_blk = index_table[index_offset];
            clear = 0;
            if(mode == 2)
                free_block(cur_index_blk);
        }
    }   
    if(clear == 1){
        bzero(tmp_blk,BLOCK_SIZE);
        write_block(tmp_blk,cur_index_blk);
    }
    if(mode == 0 && clear ==0){//mode 0下一定新增空块，clear为1
        assert(0);
    }
    return cur_index_blk;
}
void expand_file(unsigned int old_sz,unsigned int new_sz,inode_t *inode_ptr){
    unsigned int old_block = old_sz / BLOCK_SIZE + (old_sz % BLOCK_SIZE !=0);
    unsigned int new_block = new_sz / BLOCK_SIZE + (new_sz % BLOCK_SIZE !=0);
    //注意：块数与序号不同
    if(old_block == new_block){
        inode_ptr->file_size = new_sz;
        write_inode(inode_ptr,inode_ptr->inode_id);
        return ;
    } 
    unsigned int file_blk;
    //新增块：old_block ~ new_block-1
    for(file_blk = old_block;file_blk < new_block;file_blk++){
        block_index(file_blk,inode_ptr,0);
    }
    inode_ptr->file_size = new_sz;
    write_inode(inode_ptr,inode_ptr->inode_id);
}
/**********************************************/
void init_fs(){
    read_superblock();
    if(superblock.magic == SUPERBLOCK_MAGIC){
        //将SD卡中已有的有效数据拿出
        printk("    [FS] FS is existed, reuse it \n");
        do_statfs();
        read_block_map();
        read_inode_map();
        //读取根目录为当前目录
        read_inode(&cur_dir_inode,0); //在创建时根目录也将使用0号
        read_block(cur_dir_block,cur_dir_inode.direct[0]); //0号指向自身数据块
        //无论存在与否，文件描述符都需初始化
        for(int i=0;i<NUM_FDESCS;i++)
            fdesc_array[i].used = 0;
    }
    else{
        printk("    [FS] FS doesn't exist, start building...\n");
        build_fs(); //包含文件描述符初始化
    }
}

void build_fs(){
    printk("    [FS] Building superblock...\n");
    superblock.magic = SUPERBLOCK_MAGIC;
    superblock.fs_start = FS_SEC_START;
    superblock.fs_sec = FS_SEC_NUM;
    superblock.block_map_start = BLOCK_MAP_SEC_START;
    superblock.block_map_sec = BLOCK_MAP_SEC_NUM;
    superblock.inode_map_start = INODE_MAP_SEC_START;
    superblock.inode_map_sec = INODE_MAP_SEC_NUM;
    superblock.inode_start = INODE_BASE_SEC_START;
    superblock.inode_num = INODE_NUM;
    superblock.inode_free = INODE_NUM;
    superblock.block_start = BLOCK_BASE_SEC_START;
    superblock.block_num = BLOCK_NUM;
    superblock.block_free = BLOCK_NUM;
    superblock.inode_size = sizeof(inode_t);
    superblock.dentry_size = sizeof(dentry_t);
    write_superblock();
    printk("    magic: 0x%x\n",superblock.magic);
    printk("    file system start(num): %d(%d)\n",superblock.fs_start,superblock.fs_sec);
    printk("    inode map sec start(num): %d(%d)\n",superblock.inode_map_start,superblock.inode_map_sec);
    printk("    block map sec start(num): %d(%d)\n",superblock.block_map_start,superblock.block_map_sec);
    printk("    inode sec start(num): (%d)%d\n",superblock.inode_start,superblock.inode_num>>2);
    printk("    block sec start(num): (%d)%d\n",superblock.block_start,superblock.block_num<<3);
    printk("    inode size: %dB, dir entry size: %dB\n",superblock.inode_size,superblock.dentry_size);
    printk("    [FS] Building block map...\n");
    bzero(block_map_buf,superblock.block_map_sec << 9);
    write_block_map();

    printk("    [FS] Building inode map...\n");
    bzero(inode_map_buf,superblock.inode_map_sec << 9);
    write_inode_map();

    printk("    [FS] Building root dir...\n");    
    unsigned int root_id = alloc_inode();
    assert(root_id == 0);
    unsigned int root_blk_id = alloc_block();
    //init contains write back to sd card
    init_inode(&cur_dir_inode,root_id,root_blk_id,I_DIR,O_RDWR);
    init_dir_block((dentry_t *)cur_dir_block,cur_dir_inode.inode_id,root_id,root_blk_id);

    printk("    [FS] Building fdesc array...\n");
    for(int i=0;i<NUM_FDESCS;i++)
        fdesc_array[i].used = 0;
}


int do_mkfs(void)
{
    // TODO [P6-task1]: Implement do_mkfs
    printk("Start rebuilding File System!\n");
    build_fs();
    printk("Rebuild done!\n");
    return 0;  // do_mkfs succeeds
}

int do_statfs(void)
{
    // TODO [P6-task1]: Implement do_statfs
    int inode_used = superblock.inode_num - superblock.inode_free;
    int block_used = superblock.block_num - superblock.block_free;
    //关于inode占用数据块可能统计不准确，一个数据块可存16个inode
    int inode_blk_used = inode_used/16 + (inode_used % 16 != 0);
    int total_blk_used = 1+4+1+inode_blk_used+block_used;
    printk("magic: 0x%x\n",superblock.magic);
    printk("file system start(num): %d(%d)\n",superblock.fs_start,superblock.fs_sec);
    printk("inode map sec start(num): %d(%d)\n",superblock.inode_map_start,superblock.inode_map_sec);
    printk("block map sec start(num): %d(%d)\n",superblock.block_map_start,superblock.block_map_sec);
    printk("inode sec start(num): (%d)%d\n",superblock.inode_start,superblock.inode_num>>2);
    printk("block sec start(num): (%d)%d\n",superblock.block_start,superblock.block_num<<3);
    printk("inode size: %dB, dir entry size: %dB\n",superblock.inode_size,superblock.dentry_size);
    printk("Block usage: %d/%d Inode usage: %d/%d Data usage: %d/%d\n",
        total_blk_used,FS_BLK_NUM,inode_used,superblock.inode_num,block_used,superblock.block_num);
    return 0;  // do_statfs succeeds
}

int do_cd(char *path)
{
    // TODO [P6-task1]: Implement do_cd
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    char dir[DIR_LEVEL][NAME_LEN];
    int level = split_path(path,dir);
    assert(level <= DIR_LEVEL);
    int index; //目录中的项号
    int status = 0;
    for(int i=0;i<level;i++){
        index = search_dentry(cur_dir,dir[i],D_DIR);
        if(index == 0 || (index == 1 && cur_dir_inode.inode_id == 0))
            continue;
        if(index<0){
            for(int j=0;j<i;j++)
                printk("%s/",dir[j]);
            if(index == -2){
                printk("'%s': No such file or directory!\n",dir[i]);
            }  
            else if(index == -1)
                printk("'%s': Not a directory!\n",dir[i]);
            return status;            
        }
        else{
            unsigned int cd_id = cur_dir[index].inode_id;
            //write back first
            // write_inode(&cur_dir_inode,cur_dir_inode.inode_id);
            // write_block(cur_dir_block,cur_dir_inode.direct[0]);
            read_inode(&cur_dir_inode,cd_id);
            read_block(cur_dir_block,cur_dir_inode.direct[0]);
            status += (1<<i);
        }        
    }
    return status;  // do_cd succeeds
}

int do_mkdir(char *path)
{//只考虑单级目录
    // TODO [P6-task1]: Implement do_mkdir
    if(path == NULL){
        printk("Missing operand\n");
        return 0;
    }
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int dentry_num = BLOCK_SIZE / superblock.dentry_size;
    int index = search_dentry(cur_dir,path,D_DIR);
    if(index >= 0)
        printk("'%s': Directory exists!\n",path);
    else if(index == -1)
        printk("'%s': Is a file!\n",path);
    else{
        int newdir = -1;
        for(int i=2;i<dentry_num;i++)
            if(cur_dir[i].dtype == D_NULL)
            {
                newdir = i;
                break;
            }
        assert(newdir>0);
        unsigned int new_inode_id = alloc_inode();
        unsigned int new_block_id = alloc_block();
        init_inode(&tmp_inode,new_inode_id,new_block_id,I_DIR,O_RDWR);
        init_dir_block(&tmp_blk,new_inode_id,cur_dir_inode.inode_id,new_block_id);
        
        cur_dir[newdir].inode_id = new_inode_id;
        cur_dir[newdir].dtype = D_DIR;
        strcpy(cur_dir[newdir].name,path);
        write_block(cur_dir_block,cur_dir_inode.direct[0]);

        cur_dir_inode.link_cnt ++;
        cur_dir_inode.file_size += superblock.dentry_size;
        cur_dir_inode.modify_time = get_timer();
        write_inode(&cur_dir_inode,cur_dir_inode.inode_id);
    }

    return 0;  // do_mkdir succeeds
}

int do_rmdir(char *path)
{//只考虑单级目录
    // TODO [P6-task1]: Implement do_rmdir
    if(path == NULL){
        printk("Missing operand\n");
        return 0;
    }
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int index = search_dentry(cur_dir,path,D_DIR);
    if(index == -2)
        printk("'%s': No such file or directory!\n",path);
    else if(index == -1)
        printk("'%s': Not a directory!\n",path);
    else if(index == 0 || index == 1)
        printk("'%s': Illegal operand!\n",path);
    else{
        //由于不允许硬链接到目录，直接释放即可
        unsigned int rm_inode_id = cur_dir[index].inode_id;
        read_inode(&tmp_inode,rm_inode_id);
        if(tmp_inode.link_cnt > 1){
            printk("Fail! Some File remain in the Directory!\n");
            return 0;
        }
        // unsigned int rm_block_id = tmp_inode.direct[0];
        // free_block(rm_block_id);
        free_inode(rm_inode_id);
        //更改inode和目录块
        cur_dir[index].dtype = D_NULL;
        write_block(cur_dir_block,cur_dir_inode.direct[0]);
        cur_dir_inode.link_cnt --;
        cur_dir_inode.file_size -= superblock.dentry_size;
        cur_dir_inode.modify_time = get_timer();
        write_inode(&cur_dir_inode,cur_dir_inode.inode_id);
    }
    return 0;  // do_rmdir succeeds
}

int do_ls(char *path, int option)
{
    // TODO [P6-task1]: Implement do_ls
    // Note: argument 'option' serves for 'ls -l' in A-core
    char dir[DIR_LEVEL][NAME_LEN];
    printk("ls path:%s\n",path);
    int level = split_path(path,dir);
    assert(level <= DIR_LEVEL);
    int index;
    read_inode(&tmp_inode,cur_dir_inode.inode_id);
    read_block(tmp_blk,tmp_inode.direct[0]);
    dentry_t *tmp_dir = (dentry_t *)tmp_blk;
    //定位要显示的目录，level0显示当前目录
    for(int i=0;i<level;i++)
    {
        index = search_dentry(tmp_dir,dir[i],D_DIR);
        if(index == 0 || (index == 1 && tmp_inode.inode_id == 0))
            continue;
        if(index<0){
            for(int j=0;j<i;j++)
                printk("%s/",dir[j]);
            if(index == -2){
                printk("'%s': No such file or directory!\n",dir[i]);
            }  
            else if(index == -1)
                printk("'%s': Not a directory!\n",dir[i]);
            return 0;            
        }
        else{
            unsigned int ls_id = tmp_dir[index].inode_id;
            read_inode(&tmp_inode,ls_id);
            read_block(tmp_blk,tmp_inode.direct[0]);
            tmp_dir = (dentry_t *)tmp_blk;
        }
    }
    int dentry_num = BLOCK_SIZE / superblock.dentry_size;
    if(option == 0){
        for(int i=2;i<dentry_num;i++)
            if(tmp_dir[i].dtype == D_NULL)
                continue;
            else
                printk("%s\t",tmp_dir[i].name);
        printk("\n");
    }   
    else if(option == 1){
                printk("NAME\t INODE_ID\t LINK\t SIZE\n");
        for(int i=2;i<dentry_num;i++)
            if(tmp_dir[i].dtype == D_NULL)
                continue;
            else{
                printk("%s\t\t\t\t\t\t\t%d\t\t\t\t",tmp_dir[i].name,tmp_dir[i].inode_id);
                read_inode(&tmp_inode,tmp_dir[i].inode_id);
                printk("%d\t\t\t\t%d\n",tmp_inode.link_cnt,tmp_inode.file_size);
            }
    }
    return 0;  // do_ls succeeds
}

int do_touch(char *path)
{
    // TODO [P6-task2]: Implement do_touch
    if(path == NULL){
        printk("Missing operand\n");
        return 0;
    }
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int dentry_num = BLOCK_SIZE / superblock.dentry_size;
    int index = search_dentry(cur_dir,path,D_FILE);
    if(index >= 0)
        printk("'%s': File exists!\n",path);
    else if(index == -1)
        printk("'%s': Is a Directory!\n",path);
    else{
        int newfile = -1;
        for(int i=2;i<dentry_num;i++)
            if(cur_dir[i].dtype == D_NULL)
            {
                newfile = i;
                break;
            }
        assert(newfile>0);
        unsigned int new_inode_id = alloc_inode();
        init_inode(&tmp_inode,new_inode_id,0,I_FILE,O_RDWR); //初始不分配数据块
        // unsigned int new_block_id = alloc_block();
        // init_inode(&tmp_inode,new_inode_id,new_block_id,I_FILE,O_RDWR);
        // bzero(tmp_blk,0,BLOCK_SIZE);
        // write_block(tmp_blk,new_block_id);
        
        cur_dir[newfile].inode_id = new_inode_id;
        cur_dir[newfile].dtype = D_FILE;
        strcpy(cur_dir[newfile].name,path);
        write_block(cur_dir_block,cur_dir_inode.direct[0]);

        cur_dir_inode.link_cnt ++;
        cur_dir_inode.file_size += superblock.dentry_size;
        cur_dir_inode.modify_time = get_timer();
        write_inode(&cur_dir_inode,cur_dir_inode.inode_id);
    }
    return 0;  // do_touch succeeds
}

int do_cat(char *path)
{
    // TODO [P6-task2]: Implement do_cat
    //考虑显示支持，只打印第一个数据块
    if(path == NULL){
        printk("Missing operand\n");
        return 0;
    }
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int dentry_num = BLOCK_SIZE / superblock.dentry_size;
    int index = search_dentry(cur_dir,path,D_FILE);
    if(index == -2)
        printk("'%s': No such file or directory!\n",path);
    else if(index == -1)
        printk("'%s': Is a directory!\n",path);
    else if(index == 0 || index == 1)
        printk("'%s': Illegal operand!\n",path);
    else{
        read_inode(&tmp_inode,cur_dir[index].inode_id);
        if(tmp_inode.file_size > BLOCK_SIZE)
            printk("File too lage. Please see directly!\n");
        else{
            read_block(tmp_blk,tmp_inode.direct[0]);
            printk("%s\n",tmp_blk);
        }
    }
    return 0;  // do_cat succeeds
}

int do_fopen(char *path, int mode)
{
    // TODO [P6-task2]: Implement do_fopen
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int index = search_dentry(cur_dir,path,D_FILE);
    assert(index>=0);
    read_inode(&tmp_inode,cur_dir[index].inode_id);
    assert(tmp_inode.access == O_RDWR || tmp_inode.access == mode); //check access
    int fd= -1;
    for(int i=0;i<NUM_FDESCS;i++)
        if(fdesc_array[i].used == 0)
        {
            fd = i;
            fdesc_array[fd].file_inode = cur_dir[index].inode_id;
            fdesc_array[fd].access = mode;
            fdesc_array[fd].used = 1;
            fdesc_array[fd].rd_offset = 0;
            fdesc_array[fd].wr_offset = 0;
            break;
        }

    assert(fd >= 0)
    return fd;  // return the id of file descriptor
}

int do_fread(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fread
    //依次检查fd存在、权限、范围
    if(fdesc_array[fd].used == 0)
        return 0;
    if(fdesc_array[fd].access == O_WRONLY)
        return 0;
    read_inode(&tmp_inode,fdesc_array[fd].file_inode);
    if(fdesc_array[fd].rd_offset + length > tmp_inode.file_size)
        return 0;
    //对读取的内容进行分别定位，可能分别位于不同数据块
    unsigned int rd_blk,rd_offset;
    int ptr = 0;
    unsigned int index_blk;
    while(length > 0){
        rd_blk = fdesc_array[fd].rd_offset / BLOCK_SIZE;
        rd_offset = fdesc_array[fd].rd_offset % BLOCK_SIZE;
        index_blk = block_index(rd_blk,&tmp_inode,1);
        read_block(tmp_blk,index_blk);
        int cp_len = min(length,BLOCK_SIZE - rd_offset);
        memcpy(buff+ptr,tmp_blk+rd_offset,cp_len);
        ptr+=cp_len;
        fdesc_array[fd].rd_offset += cp_len;    
        length -= cp_len;
    }
    return ptr;  // return the length of trully read data
}

int do_fwrite(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_fwrite
    if(fdesc_array[fd].used == 0)
        return 0;
    if(fdesc_array[fd].access == O_RDONLY)
        return 0;
    // unsigned int max_blk = DIRECT_NUM + INDIRECT_NUM_1*INDIRECT1_INDEX + INDIRECT_NUM_2*INDIRECT2_INDEX + INDIRECT_NUM_3*INDIRECT3_INDEX;
    // unsigned int max_filesz = max_blk * BLOCK_SIZE;
    read_inode(&tmp_inode,fdesc_array[fd].file_inode);
    if(fdesc_array[fd].wr_offset + length > tmp_inode.file_size){
        expand_file(tmp_inode.file_size,fdesc_array[fd].wr_offset+length,&tmp_inode);
    }
    unsigned int wr_blk,wr_offset;
    int ptr=0;
    unsigned int index_blk;
    while(length > 0){
        wr_blk = fdesc_array[fd].wr_offset / BLOCK_SIZE;
        wr_offset = fdesc_array[fd].wr_offset % BLOCK_SIZE;
        index_blk = block_index(wr_blk,&tmp_inode,1);
        read_block(tmp_blk,index_blk);
        int cp_len = min(length,BLOCK_SIZE - wr_offset);
        memcpy(tmp_blk+wr_offset,buff+ptr,cp_len);
        write_block(tmp_blk,index_blk);
        ptr+=cp_len;
        fdesc_array[fd].wr_offset += cp_len;
        length -= length;
    }
    return ptr;  // return the length of trully written data
}

int do_fclose(int fd)
{
    // TODO [P6-task2]: Implement do_fclose
    fdesc_array[fd].used = 0;
    return 0;  // do_fclose succeeds
}

int do_ln(char *src_path, char *dst_path)
{
    // TODO [P6-task2]: Implement do_ln
    //只支持同一目录下的操作
    if(src_path == NULL || dst_path == NULL){
        printk("Missing operand\n");
        return 0;
    }
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int dentry_num = BLOCK_SIZE / superblock.dentry_size;
    int src_index = search_dentry(cur_dir,src_path,D_FILE);
    int dst_index = search_dentry(cur_dir,dst_path,D_FILE);
    unsigned int inode_id;
    if(src_index == -2){
        printk("'%s': No such file or directory!\n",src_path);
        return 0;
    }
    else if(src_index == -1){
        printk("'%s': Is a directory!\n",src_path);
        return 0;
    }
    else if(src_index == 0 || src_index == 1){
        printk("'%s': Illegal operand!\n",src_path);
        return 0;
    }
    if(dst_index == -1){
        printk("'%s': Is a Directory!\n");
        return 0;
    }
    else if(dst_index >= 0){
        printk("'%s': File exists!\n");
        return 0;
    }
    
    inode_id = cur_dir[src_index].inode_id;
    int new_link=-1;
    for(int i=2;i<dentry_num;i++)
        if(cur_dir[i].dtype == D_NULL){
            new_link = i;
            break;
        }
    assert(new_link > 0);
    cur_dir[new_link].inode_id = inode_id;
    cur_dir[new_link].dtype = D_FILE;
    strcpy(cur_dir[new_link].name,dst_path);
    write_block(cur_dir_block,cur_dir_inode.inode_id);

    cur_dir_inode.link_cnt ++;
    cur_dir_inode.file_size += superblock.dentry_size;
    cur_dir_inode.modify_time = get_timer();
    write_inode(&cur_dir_inode,cur_dir_inode.inode_id);

    read_inode(&tmp_inode,inode_id);
    tmp_inode.link_cnt++;
    write_inode(&tmp_inode,inode_id);
    return 0;  // do_ln succeeds 
}

int do_rm(char *path)
{
    // TODO [P6-task2]: Implement do_rm
    if(path == NULL){
        printk("Missing operand\n");
        return 0;
    }
    dentry_t *cur_dir = (dentry_t *)cur_dir_block;
    int index = search_dentry(cur_dir,path,D_FILE);
    if(index == -2)
        printk("'%s': No such file or directory!\n",path);
    else if(index == -1)
        printk("'%s': Not a file!\n",path);
    else if(index == 0 || index == 1)
        printk("'%s': Illegal operand!\n",path);
    else{
        //由于不允许硬链接到目录，直接释放即可
        unsigned int rm_inode_id = cur_dir[index].inode_id;
        read_inode(&tmp_inode,rm_inode_id);
        tmp_inode.link_cnt--;
        if(tmp_inode.link_cnt ==0)
            free_inode(rm_inode_id);
        else
            write_inode(&tmp_inode,rm_inode_id);
        // unsigned int rm_block_id = tmp_inode.direct[0];
        // free_block(rm_block_id);
        // free_inode(rm_inode_id);
        //更改inode和目录块
        cur_dir[index].dtype = D_NULL;
        write_block(cur_dir_block,cur_dir_inode.direct[0]);
        cur_dir_inode.link_cnt --;
        cur_dir_inode.file_size -= superblock.dentry_size;
        cur_dir_inode.modify_time = get_timer();
        write_inode(&cur_dir_inode,cur_dir_inode.inode_id);
    }
    return 0;  // do_rm succeeds 
}

int do_lseek(int fd, int offset, int whence,int rw) //0 r 1 w
{
    // TODO [P6-task2]: Implement do_lseek
    if(fdesc_array[fd].used == 0)
        return 0;
    //assert(fdesc_array[fd].rd_offset == fdesc_array[fd].wr_offset);
    read_inode(&tmp_inode,fdesc_array[fd].file_inode);
    if(whence == 0){
        ;
    }
    else if(whence == 1){
        if(rw == 0)
            offset += fdesc_array[fd].rd_offset;
        else
            offset += fdesc_array[fd].wr_offset;
    }
    else if(whence == 2){
        offset += tmp_inode.file_size;
    }
    assert(offset >= 0);
    // printk("before expand: %d %d\n",offset,tmp_inode.file_size);
    if(offset > tmp_inode.file_size)
        expand_file(tmp_inode.file_size,offset,&tmp_inode);
    // printk("after expand: %d\n",tmp_inode.file_size);
    if(rw == 0)
        fdesc_array[fd].rd_offset = offset;
    else
        fdesc_array[fd].wr_offset = offset;
    
    return offset;  // the resulting offset location from the beginning of the file
}
