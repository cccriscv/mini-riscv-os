#include "os.h"

// ref: https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h
// Intro: https://github.com/ianchen0119/AwesomeCS/wiki/2-5-RISC-V::%E4%B8%AD%E6%96%B7%E8%88%87%E7%95%B0%E5%B8%B8%E8%99%95%E7%90%86----PLIC-%E4%BB%8B%E7%B4%B9
#define PLIC_BASE 0x0c000000L
#define PLIC_PRIORITY(id) (PLIC_BASE + (id)*4)
#define PLIC_PENDING(id) (PLIC_BASE + 0x1000 + ((id) / 32))
#define PLIC_MENABLE(hart) (PLIC_BASE + 0x2000 + (hart)*0x80)
#define PLIC_MTHRESHOLD(hart) (PLIC_BASE + 0x200000 + (hart)*0x1000)
#define PLIC_MCLAIM(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)
#define PLIC_MCOMPLETE(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)

void plic_init()
{
  int hart = r_tp();
  // QEMU Virt machine support 7 priority (1 - 7),
  // The "0" is reserved, and the lowest priority is "1".
  *(uint32_t *)PLIC_PRIORITY(UART0_IRQ) = 1;

  /* Enable UART0 */
  *(uint32_t *)PLIC_MENABLE(hart) = (1 << UART0_IRQ);

  /* Set priority threshold for UART0. */

  *(uint32_t *)PLIC_MTHRESHOLD(hart) = 0;

  /* enable machine-mode external interrupts. */
  w_mie(r_mie() | MIE_MEIE);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);
}

int plic_claim()
{
  int hart = r_tp();
  int irq = *(uint32_t *)PLIC_MCLAIM(hart);
  return irq;
}

void plic_complete(int irq)
{
  int hart = r_tp();
  *(uint32_t *)PLIC_MCOMPLETE(hart) = irq;
}