#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa

#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))
//宏定义 通过字节数计算填全扇区数，但只能用于开头对齐

/* TODO: [p1-task4] design your own task_info_t */
typedef struct {
    int entry;
    int size;
    int vaddr;
    int memsz;
    char  name[20];
} task_info_t; //36 byte

#define TASK_MAXNUM 32
static task_info_t taskinfo[TASK_MAXNUM];

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr);
static uint64_t get_entrypoint(Elf64_Ehdr ehdr);
static uint32_t get_filesz(Elf64_Phdr phdr);
static uint32_t get_memsz(Elf64_Phdr phdr);
static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr);
static void write_padding(FILE *img, int *phyaddr, int new_phyaddr);
static void write_img_info(int nbytes_kernel, task_info_t *taskinfo,
                           int tasknum, FILE *img);

int info_sz; //用户信息占用位置，在kernel前,该变量所有子函数可见

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock main) */
        error("usage: %s %s\n", progname, ARGS);
    }
    //以上内容检测输入格式规范，除本程序外文件均需在子函数使用
    create_image(argc - 1, argv + 1);
    return 0;
}

/* TODO: [p1-task4] assign your task_info_t somewhere in 'create_image' */
static void create_image(int nfiles, char *files[])
{
    //两个以上程序 bootblock main及用户程序
    int tasknum = nfiles - 2;
    int nbytes_kernel = 0;
    int phyaddr = 0;
    FILE *fp = NULL, *img = NULL;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen(IMAGE_FILE, "w");
    assert(img != NULL);

    //task4: 为了制作用户信息扇区，提前遍历获取信息，但是此时不必遍历bootblock
    info_sz = 5*sizeof(int) + tasknum*sizeof(task_info_t); //用户信息占用位置，在kernel前
    int cntaddr = SECTOR_SIZE + info_sz; //从第二个扇区kernel位置开始统计位置情况
    //task3的使用中只使用一次入参，此处需要使用备份fp
    char **nfl = files;
    nfl ++; //!! 注意指针要跳过bl
    for (int fidx = 1; fidx < nfiles; ++fidx){
        int taskidx = fidx - 2; //从第三个(fidx=2)开始才是测试任务
        //record task info 
        if(taskidx>=0){
            taskinfo[taskidx].entry = cntaddr;
            taskinfo[taskidx].size = 0; //initial
            //taskinfo[taskidx].id = taskidx;
            taskinfo[taskidx].memsz =0;
            printf("---id:%d entry:%x size:%x\n",taskidx,taskinfo[taskidx].entry,taskinfo[taskidx].size);
            strcpy(taskinfo[taskidx].name,*nfl);
        }

        /* open input file */
        fp = fopen(*nfl, "r");
        assert(fp != NULL);

        /* read ELF header */
        read_ehdr(&ehdr, fp);

        /* for each program header */
        for (int ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
            //根据文件头获取程序数目，依次读取程序头
            if(phdr.p_flags % 2 == 1) //LOAD type
                printf("va: 0x%lx\n",phdr.p_vaddr);
            else
                continue;
            /* update nbytes_kernel */
            if (strcmp(*nfl, "main") == 0) {
                nbytes_kernel += get_filesz(phdr);
            }
            else if(taskidx>=0){
                taskinfo[taskidx].size += get_filesz(phdr);
                taskinfo[taskidx].vaddr = phdr.p_vaddr;
                taskinfo[taskidx].memsz += phdr.p_memsz;
            }
            //统计kernel(main)的实际大小，进而获得扇区数。用户程序数直接通过edr获得
            cntaddr += get_filesz(phdr);
        }
        fclose(fp);
        nfl ++;
    }

    /* for each input file */
    for (int fidx = 0; fidx < nfiles; ++fidx) {

        /* open input file */
        fp = fopen(*files, "r");
        assert(fp != NULL);

        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files); //入口及程序名

        /* for each program header */
        for (int ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
            //根据文件头获取程序数目，依次读取程序头
            printf("flags: %d\n",phdr.p_flags);
            if(phdr.p_flags % 2 == 1) //即可执行 LOAD type
                printf("va: 0x%lx\n",phdr.p_vaddr);
            else
                continue;
            /* write segment to the image */
            write_segment(phdr, fp, img, &phyaddr);
            //phyaddr 写到img的地址 从0开始
        }

        /* write padding bytes */
        /**
         * TODO:
         * 1. [p1-task3] do padding so that the kernel and every app program
         *  occupies the same number of sectors
         * 2. [p1-task4] only padding bootblock is allowed!
         */
        if (strcmp(*files, "bootblock") == 0) {
            //对bootblock 补全至1个扇区，注意该函数phyaddr补全至后者而非整数倍
            write_padding(img, &phyaddr, SECTOR_SIZE); 
            //task4: 用户信息扇区在bl后，kernel前
            //!! 注意，需要更新phyaddr
            write_img_info(nbytes_kernel, taskinfo, tasknum, img);
            phyaddr = SECTOR_SIZE + info_sz;
        }
        //task3:
            //对kernel以及用户程序，task3中设置15个扇区。此处需将phyaddr置为下一个程序写至image的地址
        // else{
        //     write_padding(img, &phyaddr, SECTOR_SIZE + fidx*15*SECTOR_SIZE);
        // }
        
        fclose(fp);
        files++;
    }
    

    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    int ret;

    ret = fread(ehdr, sizeof(*ehdr), 1, fp);
    assert(ret == 1);
    assert(ehdr->e_ident[EI_MAG1] == 'E');
    assert(ehdr->e_ident[EI_MAG2] == 'L');
    assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    int ret;

    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    //从file头开始偏移一定位置，获取程序头
    ret = fread(phdr, sizeof(*phdr), 1, fp);
    assert(ret == 1);
    if (options.extended == 1) {
        printf("\tsegment %d\n", ph);
        printf("\t\toffset 0x%04lx", phdr->p_offset);
        printf("\t\tvaddr 0x%04lx\n", phdr->p_vaddr);
        printf("\t\tfilesz 0x%04lx", phdr->p_filesz);
        printf("\t\tmemsz 0x%04lx\n", phdr->p_memsz);
    }
}

static uint64_t get_entrypoint(Elf64_Ehdr ehdr)
{
    return ehdr.e_entry;
}

static uint32_t get_filesz(Elf64_Phdr phdr)
{
    return phdr.p_filesz;
}

static uint32_t get_memsz(Elf64_Phdr phdr)
{
    return phdr.p_memsz;
}

static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr)
{
    if (phdr.p_memsz != 0 && phdr.p_type == PT_LOAD) {
        /* write the segment itself */
        /* NOTE: expansion of .bss should be done by kernel or runtime env! */
        if (options.extended == 1) {
            printf("\t\twriting 0x%04lx bytes\n", phdr.p_filesz);
        }
        fseek(fp, phdr.p_offset, SEEK_SET);
        while (phdr.p_filesz-- > 0) {
            fputc(fgetc(fp), img);
            (*phyaddr)++;
        }
        //从程序位置逐字节写到image
    }
}

static void write_padding(FILE *img, int *phyaddr, int new_phyaddr)
{
    if (options.extended == 1 && *phyaddr < new_phyaddr) {
        printf("\t\twrite 0x%04x bytes for padding\n", new_phyaddr - *phyaddr);
    }

    while (*phyaddr < new_phyaddr) {
        fputc(0, img);
        (*phyaddr)++;
    }
}

static void write_img_info(int nbytes_kern, task_info_t *taskinfo,
                           int tasknum, FILE * img)
{
    // TODO: [p1-task3] & [p1-task4] write image info to some certain places
    // NOTE: os size, infomation about app-info sector(s) ...
        //task3: only kernel is need, and pad it
    // short sec = NBYTES2SEC(nbytes_kern);
    // fseek(img,OS_SIZE_LOC,SEEK_SET);
    // fwrite(&sec,2,1,img);
    
        //task4: 需要一个扇区用于记录，镜像放在kernel前，内存放在第一个任务处。
    //os_size用于记录该扇区的大小,位于第一个扇区尾部，写2个short之后进入该扇区
    //以下为扇区内部信息分布
    //kernel block id || kernel block num || kernel entry_offset || kernel tail_offset || tasknum || taskinfo1 || ...
    //detailed taskinfo will be get in loader by entry and size
    fseek(img,OS_SIZE_LOC,SEEK_SET);
    short info_sec = NBYTES2SEC(info_sz);
    fwrite(&info_sec,2,1,img);
    fputc(BOOT_LOADER_SIG_1,img);
    fputc(BOOT_LOADER_SIG_2,img);
    //以下为第二个扇区
    int kernel_entry = SECTOR_SIZE + info_sz;
    int kernel_block_id = kernel_entry / SECTOR_SIZE;
    int kernel_tail_id = (kernel_entry + nbytes_kern) / SECTOR_SIZE;
    int kernel_block_num = kernel_tail_id - kernel_block_id +1;
    //offset from kernel addr in memory
    int kernel_entry_offset = kernel_entry - kernel_block_id*SECTOR_SIZE;
    int kernel_tail_offset = kernel_entry_offset + nbytes_kern;
    printf("tail offset %d\n",kernel_tail_offset);
    fwrite(&kernel_block_id,4,1,img);
    fwrite(&kernel_block_num,4,1,img);
    fwrite(&kernel_entry_offset,4,1,img);
    fwrite(&kernel_tail_offset,4,1,img);
    fwrite(&tasknum,4,1,img);
    for(int i=0;i<tasknum;i++) 
        fwrite(&taskinfo[i],sizeof(task_info_t),1,img);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
