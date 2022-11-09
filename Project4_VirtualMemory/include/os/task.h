#ifndef __INCLUDE_TASK_H__
#define __INCLUDE_TASK_H__

#include <type.h>

#define TASK_MEM_BASE    0x52000000
#define TASK_MAXNUM      32
#define TASK_SIZE        0x10000

/* TODO: [p1-task4] implement your own task_info_t! */
typedef struct {
    int entry;
    int size;
    int vaddr;
    int memsz;
    char  name[20];
} task_info_t; //36 byte

extern short tasknum; //set for main.c and loader.c
extern task_info_t tasks[TASK_MAXNUM];


#endif