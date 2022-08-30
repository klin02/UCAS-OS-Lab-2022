#include <asm/biosdef.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50201000

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

// use to fill unused ports when call sbi
.equ ignore, 0x0

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
	li 	a0, BIOS_SDREAD
	li 	a1, kernel
	la 	a2, os_size_loc
	lh 	a2, 0(a2)
	li 	a3, 1	// block id start from 0
	li 	a4, ignore
	li 	a5, ignore
	call 	bios_func_entry

	// TODO: [p1-task4] load task-related arguments and pass them to kernel


	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	 jal 	kernel  //kernel is addr, no func

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

