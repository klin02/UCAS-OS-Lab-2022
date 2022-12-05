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
#define SYSCALL_READCH 21 //读取字符
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_CLEAR 24
#define SYSCALL_BACKSPACE 25 //回退
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_LOCK_INIT 40
#define SYSCALL_LOCK_ACQ 41
#define SYSCALL_LOCK_RELEASE 42

//取0-63中未被使用过的数,保持和tiny_libc/include/syscall.h一致
#define SYSCALL_PTHREAD_CREATE 60
#define SYSCALL_PTHREAD_JOIN 61

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
#define SYSCALL_SHM_GET 56
#define SYSCALL_SHM_DT 57
#define SYSCALL_NET_SEND 63
#define SYSCALL_NET_RECV 64
#define SYSCALL_FS_MKFS 65
#define SYSCALL_FS_STATFS 66
#define SYSCALL_FS_CD 67
#define SYSCALL_FS_MKDIR 68
#define SYSCALL_FS_RMDIR 69
#define SYSCALL_FS_LS 70
#define SYSCALL_FS_TOUCH 71
#define SYSCALL_FS_CAT 72
#define SYSCALL_FS_FOPEN 73
#define SYSCALL_FS_FREAD 74 
#define SYSCALL_FS_FWRITE 75 
#define SYSCALL_FS_FCLOSE 76
#define SYSCALL_FS_LN 77
#define SYSCALL_FS_RM 78
#define SYSCALL_FS_LSEEK 79

#define SYSCALL_RUNMASK   58
#define SYSCALL_SETMASK   59

#define SYSCALL_SNAP_INIT 10
#define SYSCALL_SNAP_SHOT 11
#define SYSCALL_VA2PA     12
#endif