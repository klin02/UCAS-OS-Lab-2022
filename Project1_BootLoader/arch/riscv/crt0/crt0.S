#include <asm.h>

#define USER_STACK 0x52300000
ENTRY(_start)

    //store ra to return kernel later  
    la a0, USER_STACK
    sw ra, 0(a0)

    /* TODO: [p1-task3] setup C runtime environment for the user program */
    la  a0, __bss_start
    la  a1, __BSS_END__
    Clear_Bss:
    sw  zero, (a0)  //zero is the register 0
    addi a0, a0, 4
    blt  a0, a1, Clear_Bss

    /* TODO: [p1-task3] enter main function */
    //la  sp, USER_STACK
    jal main

    /* TODO: [p1-task3] finish task and return to the kernel */
    /* NOTE: You need to replace this with new mechanism in p3-task2! */
    //regain ra to return
    la a0, USER_STACK
    lw ra, 0(a0)
    ret

// while(1) loop, unreachable here
//loop:
  //  wfi
  //  j loop

END(_start)
