/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

/* maybe you need another stack for booting? */
//此处无需两个栈,从核将停在bootblock等待主核初始化结束,因此可直接复用同一个栈
.equ boot_stack_top_base, 0x50202000

.section ".entry_function","ax"
ENTRY(_boot)
  /*
   * this is the entrypoint of the kernel. 
   * Dont clear BSS here
   */

  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  la sp, boot_stack_top_base

  csrr a0, CSR_MHARTID 

  // this will never return
  call boot_kernel


loop:
  wfi
  j loop

END(_boot)
