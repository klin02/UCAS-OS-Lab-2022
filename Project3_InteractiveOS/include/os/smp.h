#ifndef SMP_H
#define SMP_H

#define NR_CPUS 2
extern void smp_init();
extern void wakeup_other_hart();
extern uint64_t get_current_cpu_id();
extern void lock_kernel();
extern void unlock_kernel();

extern void disable_sw_interrupt(void);
extern void clear_sw_interrupt(void);
extern void enable_sw_interrupt(void);

#endif /* SMP_H */
