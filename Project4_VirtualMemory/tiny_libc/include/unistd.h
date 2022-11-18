#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stdint.h>
typedef int32_t pid_t;
typedef pid_t pthread_t;

void sys_sleep(uint32_t time);
void sys_yield(void);
void sys_write(char *buff);
void sys_move_cursor(int x, int y);
void sys_reflush(void);
long sys_get_timebase(void);
long sys_get_tick(void);

int sys_mutex_init(int key);
void sys_mutex_acquire(int mutex_idx);
void sys_mutex_release(int mutex_idx);
// 多锁机制
// void sys_mutex_init(int key);
// void sys_mutex_acquire(void);
// void sys_mutex_release(void);

void sys_pthread_create(pthread_t *thread, void (*start_routine)(void*),void *arg);
int sys_pthread_join(pthread_t thread);

/* TODO: [P3 task1] ps, getchar */
void sys_ps(void);
int  sys_getchar(void);
void sys_backspace(void);
void sys_clear(void);

/* TODO: [P3 task1] exec, exit, kill waitpid */
// S-core
// pid_t  sys_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
// A/C-core
pid_t  sys_exec(char *name, int argc, char **argv);
void sys_runmask(char *name, int argc, char **argv,int mask);
void sys_setmask(pid_t pid, int mask);

void sys_exit(void);
int  sys_kill(pid_t pid);
int  sys_waitpid(pid_t pid);
pid_t sys_getpid();


/* TODO: [P3 task2] barrier */ 
int  sys_barrier_init(int key, int goal);
void sys_barrier_wait(int bar_idx);
void sys_barrier_destroy(int bar_idx);

/* TODO: [P3 task2] condition */ 
int sys_condition_init(int key);
void sys_condition_wait(int cond_idx, int mutex_idx);
void sys_condition_signal(int cond_idx);
void sys_condition_broadcast(int cond_idx);
void sys_condition_destroy(int cond_idx);

/* TODO: [P3 task2] mailbox */ 
int sys_mbox_open(char * name);
void sys_mbox_close(int mbox_id);
int sys_mbox_send(int mbox_idx, void *msg, int msg_length);
int sys_mbox_recv(int mbox_idx, void *msg, int msg_length);

/* TODO: [P4-task5] shmpageget/dt */
/* shmpageget/dt */
void* sys_shmpageget(int key);
void sys_shmpagedt(void *addr);

#endif
