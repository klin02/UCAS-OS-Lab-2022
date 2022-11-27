// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 SiFive
 * Copyright (C) 2018 Christoph Hellwig
 */

/*
 * This driver implements a version of the RISC-V PLIC with the actual layout
 * specified in chapter 8 of the SiFive U5 Coreplex Series Manual:
 *
 *     https://static.dev.sifive.com/U54-MC-RVCoreIP.pdf
 *
 * The largest number supported by devices marked as 'sifive,plic-1.0.0', is
 * 1024, of which device 0 is defined as non-existent by the RISC-V Privileged
 * Spec.
 */

#include <type.h>
#include <io.h>
#include <csr.h>
#include <os/irq.h>
#include <printk.h>

#include <plic.h>

static void *plic_regs;

//功能：使能/禁用 第hwirq个中断
static void plic_toggle(struct plic_handler *handler, int hwirq, int enable)
{
	//每32个源使用4字节
	uint32_t *reg = handler->enable_base + (hwirq / 32) * sizeof(uint32_t);
	uint32_t hwirq_mask = 1 << (hwirq % 32);

	if (enable)
		writel(readl(reg) | hwirq_mask, reg);
	else
		writel(readl(reg) & ~hwirq_mask, reg);
}

static void plic_irq_unmask(int hwirq)
{
	//设置优先级为1
	int enable = 1;
	writel(enable, plic_regs + PRIORITY_BASE + hwirq * PRIORITY_PER_ID);
    struct plic_handler *handler = &plic_handlers;
//如果handler有效，则使能该中断源
    if (handler->present) plic_toggle(handler, hwirq, enable);
}

/*
 * Handling an interrupt is a two-step process: first you claim the interrupt
 * by reading the claim register, then you complete the interrupt by writing
 * that source ID back to the same claim register.  This automatically enables
 * and disables the interrupt, so there's nothing else to do.
 */
//获取中断源号
uint32_t plic_claim(void)
{
	struct plic_handler *handler = &plic_handlers;
	void *claim = handler->hart_base + CONTEXT_CLAIM;

	return readl(claim);
}

//通知相应中断源完成工作
void plic_complete(int hwirq)
{
	struct plic_handler *handler = &plic_handlers;

	writel(hwirq, handler->hart_base + CONTEXT_CLAIM);
}

//nr irqs为中断源
int plic_init(uint64_t plic_regs_addr, uint32_t nr_irqs)
{
    plic_regs = (void *)plic_regs_addr;

    struct plic_handler *handler;
    int hwirq;
    uint32_t threshold = 0;

    handler = &plic_handlers;
    if (handler->present) {
        printk("handler already present.\n");
        threshold = 0xffffffff;
        goto done;
    }

    handler->present     = true;
//上下文0的机制，含门限，claim，complete
    handler->hart_base   = plic_regs + CONTEXT_BASE;
//使能位基址
    handler->enable_base = plic_regs + ENABLE_BASE;

done:
    /* priority must be > threshold to trigger an interrupt */
    writel(threshold, handler->hart_base + CONTEXT_THRESHOLD); // 上下文0中门限为0，任何均可触发
    for (hwirq = 1; hwirq <= nr_irqs; hwirq++) plic_toggle(handler, hwirq, 0); //禁用所有中断源

	if (hwirq > PLIC_E1000_QEMU_IRQ) //qemu模式
	{
		// Set the priority for PLIC_E1000_IRQ on QEMU
		//设置优先级，最低优先级
		plic_irq_unmask(PLIC_E1000_QEMU_IRQ);		
	}
	else //上板模式
	{
		// Set the priority for PLIC_E1000_IRQ on PYNQ
		plic_irq_unmask(PLIC_E1000_PYNQ_IRQ);		
	}

    return 0;
}
