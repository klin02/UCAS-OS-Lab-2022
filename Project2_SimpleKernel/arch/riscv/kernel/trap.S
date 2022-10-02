#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)

  /* TODO: [p2-task3] save exception_handler_entry into STVEC */
  # 最后两位为0，已经是direct模式
  la t0, exception_handler_entry
  csrw CSR_STVEC,t0
  # 设置用户态？
  # li t0, SR_SPIE & ~SR_SPP
  # csrw CSR_SSTATUS,t0
  /* TODO: [p2-task4] enable interrupts globally */
  jr ra
ENDPROC(setup_exception)
