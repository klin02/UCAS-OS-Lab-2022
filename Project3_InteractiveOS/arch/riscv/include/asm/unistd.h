#ifndef __ASM_UNISTD_H__
#define __ASM_UNISTD_H__

#define SYSCALL_EXEC 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3
#define SYSCALL_WAITPID 4
#define SYSCALL_PS 5
#define SYSCALL_GETPID 6
#define SYSCALL_YIELD 7
#define SYSCALL_WRITE 20
#define SYSCALL_READCH 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_CLEAR 24
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_LOCK_INIT 40
#define SYSCALL_LOCK_ACQ 41
#define SYSCALL_LOCK_RELEASE 42

//取0-63中未被使用过的数,保持和tiny_libc/include/syscall.h一致
#define SYSCALL_THREAD_CREATE 60
#define SYSCALL_THREAD_RECYCLE 61

#define SYSCALL_SHOW_TASK 43
#define SYSCALL_BARR_INIT 44
#define SYSCALL_BARR_WAIT 45
#define SYSCALL_BARR_DESTROY 46
#define SYSCALL_COND_INIT 47
#define SYSCALL_COND_WAIT 48
#define SYSCALL_COND_SIGNAL 49
#define SYSCALL_COND_BROADCAST 50
#define SYSCALL_COND_DESTROY 51
#define SYSCALL_MBOX_OPEN 52
#define SYSCALL_MBOX_CLOSE 53
#define SYSCALL_MBOX_SEND 54
#define SYSCALL_MBOX_RECV 55
#endif