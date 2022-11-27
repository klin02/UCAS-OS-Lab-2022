#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)

  /* TODO: [p2-task3] save exception_handler_entry into STVEC */
  # 最后两位为0，已经是direct模式
  la t0, exception_handler_entry
  csrw CSR_STVEC,t0

  /* TODO: [p2-task4] enable interrupts globally */
  # 需要设置sie和sstatus
  li t0, SR_SIE | SR_SUM
  csrw CSR_SSTATUS,t0

  # 核间中断，时间中断，中断控制器中断
  # li t0, SIE_SSIE | SIE_STIE | SIE_SEIE
  li t0, SIE_STIE | SIE_SSIE
  csrw CSR_SIE,t0

  # li t0, SIE_SEIE
  # csrs CSR_SIE,t0
    
  jr ra
ENDPROC(setup_exception)
