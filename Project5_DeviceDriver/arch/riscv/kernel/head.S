/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

//取栈顶,由于其过大,加载需要使用ld
#define KERNEL_STACK_0		0xffffffc052001000
#define KERNEL_STACK_1    0xffffffc052002000

ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero
  li t0, SR_SUM
  csrs CSR_SSTATUS, t0
  
  csrr a0, CSR_MHARTID
  bnez a0, second_core
  
  /* TODO: [p1-task2] clear BSS for flat non-ELF images */
  la  a0, __bss_start
  la  a1, __BSS_END__
Clear_Bss:
  sw  zero, (a0)  //zero is the register 0
  addi a0, a0, 4
  blt  a0, a1, Clear_Bss

  /* TODO: [p1-task2] setup C environment */
  ld    sp, pid0_stack
  call  main
  j     loop

second_core:
  ld    sp, pid1_stack
  call  main

loop:
  wfi   // wait for interrupts
  j loop

END(_start)
