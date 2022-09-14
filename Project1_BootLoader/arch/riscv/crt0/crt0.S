#include <asm.h>

//为了使不同任务程序同时处于运行状态，将返回地址和占用栈帧号存于其栈帧中。
//因此获取空闲栈帧信息，再进行存储

#define STACK_NUM 16
//存储空闲栈帧号和数据的位置
#define STACK_TAG_BASE 0x52300000
#define STACK_ADDR_BASE 0x52310000
// #define RA_TMP 0x53000000
ENTRY(_start)

    //store ra to return kernel later  
    # la a0, RA_TMP
    # sw ra, 0(a0)

    /* TODO: [p1-task3] setup C runtime environment for the user program */
    la  a0, __bss_start
    la  a1, __BSS_END__
    Clear_Bss:
    sw  zero, (a0)  //zero is the register 0
    addi a0, a0, 4
    blt  a0, a1, Clear_Bss

    //additional: get free task_stack by task_tag
    li  a0,STACK_NUM
    la  a1,STACK_TAG_BASE
    la  a2,STACK_ADDR_BASE
    //获取队列中空闲的栈帧
    move 	a3, zero
  stack_begin:
    bge a3,a0,return_pt
    lb  a4,0(a1)
    ld  a5,0(a2)
    beq a4,zero,stack_end
    addi a1,a1,1
    addi a2,a2,8
    j   stack_begin
  stack_end:

  //then a5 is the stack addr assigned for task program, a4 is the task_tag and a1 is the addr
    //mark it occupied
    addi a4,a4,1
    sb   a4,0(a1)
    //old sp should be stored in new stack frame to get back
    move a0,sp
    //set the stack pointer
    move sp,a5
    //store ra and a1(tag_addr) and old sp
    sd  ra,0(sp)
    sd  a1,-8(sp)
    sd  a0,-16(sp)   //old sp
    addi sp,sp,-24
    call main
    addi sp,sp,24
    ld  ra,0(sp)
    ld  a1,-8(sp)
    ld  sp,-16(sp)
    //mark it free
    sb  zero,0(a1)

//return: when no stack is free, return directly
return_pt:
    ret

// while(1) loop, unreachable here
//loop:
  //  wfi
  //  j loop
  
END(_start)
