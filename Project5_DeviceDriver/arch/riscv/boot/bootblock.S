#include <asm/biosdef.h>
#include <csr.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50202000

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

// use to fill unused ports when call sbi
.equ ignore, 0x0

// addr to store task-related arguments for kernel, behind all app in memeory
.equ user_info_addr, 0x52400000

.text
.global main

main:
	// fence on all memory and I/O
	fence

    	// a0 is mhartid
	bnez a0, secondary

	# 此处默认usr info addr占据不会超过64个扇区，即32678B
	// TODO: [p1-task4] load task-related arguments and pass them to kernel
	li 	a0, BIOS_SDREAD
	li 	a1, user_info_addr
	la 	a2, os_size_loc
	lh	a2, 0(a2) 	//get sector of app-info
	li 	a3, 1
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry

	# 判断每次bios read不超过64
	li 	t0, 64  	
# 	# 每次存储64次个扇区后，内存地址后移
# 	li 	t2, 32768
# 	// then kernel info is reachable in memory
# 	li 	a0, user_info_addr
# 	lh 	a3, 0(a0)  //kernel_block_id
# 	lh 	t1, 2(a0)  //kernel_block_num
# 	li 	a0, BIOS_SDREAD
# 	li 	a1, kernel
# 	li 	a4, ignore
# 	li 	a5, ignore	
# kernel_sector_judge: #判断剩余扇区数是否小于64
# 	blt 	t1, t0, kernel_sector_rest_load
# 	li 	a2, 64
# 	call 	bios_func_entry
# 	add 	a1, a1, t2    # 512B x 64 sector
# 	addi 	t1, t1, -64
# 	addi 	a3, a3, 64 	 # 扇区号相应添加
# 	j 	kernel_sector_judge
# kernel_sector_rest_load:
# 	addi 	a2, t1, 0
# 	call 	bios_func_entry

	li 	a0, user_info_addr
	lw 	a3, 0(a0)  //kernel_block_id
	lw 	a2, 4(a0)  //kernel_block_num
	li 	a0, BIOS_SDREAD
	li 	a1, kernel
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry
	

	// then kernel is in memory, need to be remove ahead by offset
	li 	a0, kernel
	li 	a4, user_info_addr
	lw 	a1, 8(a4) //kernel_entry_offset
	lw 	a2, 12(a4) //kernel_tail_offset
	add 	a1, a1, a0 //entry real addr
	add 	a2, a2, a0 //tail real addr
Move_Kn:
	bge 	a1, a2, Clear_Tail
	lw 	a4, 0(a1)
	sw 	a4, 0(a0)
	addi 	a1, a1, 1
	addi 	a0, a0, 1
	j 	Move_Kn
Clear_Tail:
	bge 	a0, a2, End_Mv
	sw 	zero, 0(a0)
	addi 	a0, a0, 1
	j 	Clear_Tail
End_Mv:

	// then the entry of kernel is the same as addr

	li 	a0, BIOS_PUTSTR
	la 	a1, msg
	li 	a2, ignore
	li 	a3, ignore
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry
	
	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	jal 	kernel  //kernel is addr, no func
	j 	stop 

secondary:
	/* TODO [P3-task3]: 
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */

	# 屏蔽所有中断：需同时设置SIP和SIE寄存器
	csrw 	CSR_SIE, zero
	csrw 	CSR_SIP, zero
	# 设置中断入口
	li 	t0, kernel
	csrw 	CSR_STVEC, t0
	# 允许软件中断
	li 	t0, SIE_SSIE
	csrs 	CSR_SIE, t0
	li t0, SR_SIE
	csrw CSR_SSTATUS, t0

wait_for_wakeup:
	wfi
	j wait_for_wakeup


// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

