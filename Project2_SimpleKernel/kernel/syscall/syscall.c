#include <sys/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    /* TODO: [p2-task3] handle syscall exception */
    /**
     * HINT: call syscall function like syscall[fn](arg0, arg1, arg2),
     * and pay attention to the return value and sepc
     */
    //针对syscall处理，应当增加epc，同时通过syscall表调用相关函数
    //注意，相关参数在用户态时invoke函数ecall时存入，并在例外入口函数中保存到了regs中
    regs->sepc +=4;
    //regs[10-17] 对应 a0-a7 ，此处a0既是输入又作为返回值
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10],
                                             regs->regs[11],
                                             regs->regs[12],
                                             regs->regs[13],
                                             regs->regs[14]
                                            );
    
}
