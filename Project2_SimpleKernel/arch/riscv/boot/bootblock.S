#include <asm/biosdef.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50201000

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

	// TODO: [p1-task1] call BIOS to print string "It's bootblock!"
		// call_bios((long)BIOS_PUTSTR, (long)str, IGNORE, IGNORE, IGNORE, IGNORE);
	li 	a0, BIOS_PUTSTR
	la 	a1, msg
	li 	a2, ignore
	li 	a3, ignore
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry

	// TODO: [p1-task2] call BIOS to read kernel in SD card
		// call_bios((long)BIOS_SDREAD, (long)mem_address, (long)num_of_blocks, (long)block_id, IGNORE, IGNORE);
	# li 	a0, BIOS_SDREAD
	# li 	a1, kernel
	# la 	a2, os_size_loc
	# lh 	a2, 0(a2)
	# li 	a3, 1	// block id start from 0
	# li 	a4, ignore
	# li 	a5, ignore
	# call 	bios_func_entry

	// TODO: [p1-task4] load task-related arguments and pass them to kernel
	li 	a0, BIOS_SDREAD
	li 	a1, user_info_addr
	la 	a2, os_size_loc
	lh	a2, 0(a2) 	//get sector of app-info
	li 	a3, 1
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry
	
	// then kernel info is reachable in memory
	li 	a0, user_info_addr
	lh 	a3, 0(a0)  //kernel_block_id
	lh 	a2, 2(a0)  //kernel_block_num
	li 	a0, BIOS_SDREAD
	li 	a1, kernel
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry

	// then kernel is in memory, need to be remove ahead by offset
	li 	a0, kernel
	li 	a4, user_info_addr
	lh 	a1, 4(a4) //kernel_entry_offset
	lh 	a2, 6(a4) //kernel_tail_offset
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

	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	 jal 	kernel  //kernel is addr, no func

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

