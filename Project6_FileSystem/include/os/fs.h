#ifndef __INCLUDE_OS_FS_H__
#define __INCLUDE_OS_FS_H__

/************************** MEMORY LAYOUT ****************************

------------------------------- KERNEL ------------------------------
|     NAME      |    SECTOR NUM    |  SECTOR START  |  SECTOR END   |
|   bootblock   |        1         |        0       |       0       |
|    appinfo    |        1         |        1       | 0(os_size_loc)|
|    kernel     |        X         |        X       |       X       |

----------------------------FILE SYSTEM------------------------------
|    PART NAME   |     BLOCK NUM    |  SECTOR START  |   SECTOR END  |
|   super block  |        1         |     2^20       |    2^20+7     |
|    block map   |        4         |     2^20+8     |    2^20+39    |
|    inode map   |        1         |     2^20+40    |    2^20+47    |
|      inode     |      1024        |     2^20+48    |    2^20+8239  |
|      data      |        X         |     2^20+8240  |    2^21-1     |

PS：
    1. X means no need to care
    2. 4KB = 8 sec，512MB = 2^20 sec
    3. block map is for 512MB fs, i.e. 512*256 blocks, need 32K 
********************************************************************/

#include <type.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

/* macros of file system */
#define SUPERBLOCK_MAGIC 0x20221205
#define NUM_FDESCS 16
#define NAME_LEN 20
#define DIR_LEVEL 3 //cd和ls操作允许的目录层级
//可用数量 —— FS总大小为128K块，实际可用数据块小于该值
#define FS_BLK_NUM (1<<17) // 512MB -> 128 K block * 4KB/block 
#define INODE_NUM (1<<15) //4K map * 8/byte -> 32K inode
#define BLOCK_NUM (FS_BLK_NUM-1-4-1-1024) 
//索引信息
#define DIRECT_NUM 11
#define INDIRECT_NUM_1 3
#define INDIRECT_NUM_2 2
#define INDIRECT_NUM_3 1
#define INDEX_NUM (BLOCK_SIZE / sizeof(unsigned int))
#define INDIRECT1_INDEX INDEX_NUM
#define INDIRECT2_INDEX (INDIRECT1_INDEX * INDEX_NUM)
#define INDIRECT3_INDEX (INDIRECT2_INDEX * INDEX_NUM)
//文件布局
#define FS_SEC_NUM (1<<20)
#define SUPER_BLOCK_SEC_NUM (1<<3)
#define BLOCK_MAP_SEC_NUM (4<<3)
#define INODE_MAP_SEC_NUM (1<<3)
#define INODE_BASE_SEC_NUM (1024<<3)
#define FS_SEC_START (1<<20)                                                //offset from fs_start
#define SUPER_BLOCK_SEC_START (FS_SEC_START + 0)                            //0
#define BLOCK_MAP_SEC_START (SUPER_BLOCK_SEC_START + SUPER_BLOCK_SEC_NUM)   //8
#define INODE_MAP_SEC_START (BLOCK_MAP_SEC_START + BLOCK_MAP_SEC_NUM)       //40
#define INODE_BASE_SEC_START (INODE_MAP_SEC_START + INODE_MAP_SEC_NUM)      //48
#define BLOCK_BASE_SEC_START (INODE_BASE_SEC_START + INODE_BASE_SEC_NUM)     //8240

#define BLOCK_MAP_BYTE_SZ  (4 << 12) //末尾超出BLOCK_NUM部分不可用
#define INODE_MAP_BYTE_SZ  (1 << 12)
#define BLOCK_SIZE 4096

typedef enum inode_type{
    I_DIR,
    I_FILE,
} inode_type_t;

typedef enum dentry_type{
    D_NULL,
    D_DIR,
    D_FILE,
} dentry_type_t;

typedef enum file_access{
    O_RDWR,
    O_RDONLY,
    O_WRONLY,
} file_access_t;

/* data structures of file system */
typedef struct superblock{
    // TODO [P6-task1]: Implement the data structure of superblock
    // 以下start都指起始扇区号
    unsigned int magic;
    unsigned int fs_start;
    unsigned int fs_sec;

    unsigned int block_map_start;
    unsigned int block_map_sec;

    unsigned int inode_map_start;
    unsigned int inode_map_sec;

    unsigned int inode_start;
    unsigned int inode_num;
    unsigned int inode_free;

    unsigned int block_start;
    unsigned int block_num;
    unsigned int block_free;

    unsigned int inode_size;
    unsigned int dentry_size;

} superblock_t; // less than 512B

typedef struct dentry{
    // TODO [P6-task1]: Implement the data structure of directory entry
    unsigned int inode_id;
    dentry_type_t dtype;  //类别
    char name[24]; //padding to 32B
} dentry_t;

typedef struct inode{ 
    // TODO [P6-task1]: Implement the data structure of inode
    //标志信息
    unsigned int inode_id;
    unsigned int owner_pid;
    //类别与权限
    inode_type_t type;
    file_access_t access;
    //统计信息
    unsigned int link_cnt;
    unsigned int file_size;
    unsigned int create_time;
    unsigned int modify_time;
    //索引
    unsigned int direct[DIRECT_NUM];
    unsigned int indirect1[INDIRECT_NUM_1];
    unsigned int indirect2[INDIRECT_NUM_2];
    unsigned int indirect3[INDIRECT_NUM_3];
    //对齐补全
    unsigned int padding[7]; //padding to 128B
} inode_t;

typedef struct fdesc{
    // TODO [P6-task2]: Implement the data structure of file descriptor
    unsigned int file_inode;
    file_access_t access;
    unsigned int used;
    unsigned int rd_offset;
    unsigned int wr_offset;
} fdesc_t;

/* access of inode */
#define I_DIR    0
#define I_FILE   1

/* type of dentry */
#define D_NULL   0
#define D_DIR    1
#define D_FILE   2

/* modes of do_fopen */
#define O_RDONLY 1  /* read only open */
#define O_WRONLY 2  /* write only open */
#define O_RDWR   3  /* read/write open */

/* whence of do_lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

//global variable
extern superblock_t superblock;
extern fdesc_t fdesc_array[NUM_FDESCS];

/* fs function declarations */
extern int do_mkfs(void);
extern int do_statfs(void);
extern int do_cd(char *path);
extern int do_mkdir(char *path);
extern int do_rmdir(char *path);
extern int do_ls(char *path, int option);
extern int do_touch(char *path);
extern int do_cat(char *path);
extern int do_fopen(char *path, int mode);
extern int do_fread(int fd, char *buff, int length);
extern int do_fwrite(int fd, char *buff, int length);
extern int do_fclose(int fd);
extern int do_ln(char *src_path, char *dst_path);
extern int do_rm(char *path);
extern int do_lseek(int fd, int offset, int whence,int rw);

/* other function */
extern void read_superblock();
extern void write_superblock();
extern void read_block_map();
extern void write_block_map();
extern void read_inode_map();
extern void write_inode_map();
extern void read_inode(inode_t *dest,unsigned int inode_id);
extern void write_inode(inode_t *src,unsigned int inode_id);
extern void read_block(char *dest,unsigned int block_id);
extern void write_block(char *src,unsigned int block_id);
extern unsigned int alloc_inode();
extern unsigned int alloc_block();
extern void free_inode(unsigned int inode_id);
extern void free_block(unsigned int block_id);
extern void init_inode(inode_t* inode_ptr,unsigned int inode_id,unsigned int block_id,inode_type_t type,file_access_t access);
extern void init_dir_block(dentry_t *dir,unsigned int self_id,unsigned int parent_id,unsigned int self_blk_id);
extern int search_dentry(dentry_t *dir,char *path,dentry_type_t dtype);
extern int split_path(char* path,char dir[DIR_LEVEL][NAME_LEN]);
extern unsigned int block_index(unsigned int file_blk,inode_t *inode,int mode);
extern void expand_file(unsigned int old_sz,unsigned int new_sz,inode_t *inode_ptr);
extern void init_fs();
extern void build_fs();

#endif