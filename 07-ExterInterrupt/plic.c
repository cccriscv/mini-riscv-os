#include "os.h"

void plic_init()
{
  int hart = r_mhartid();

  // QEMU Virt machine support 7 priority (1 - 7),
  // The "0" is reserved, and the lowest priority is "1".
  *(uint32_t *)PLIC_PRIORITY(UART0_IRQ) = 1;

  /* Enable UART0 */
  *(uint32_t *)PLIC_MENABLE(hart) = (1 << UART0_IRQ);

  /* Set priority threshold for UART0. */

  *(uint32_t *)PLIC_MTHRESHOLD(hart) = 0;

  /* enable machine-mode external interrupts. */
  w_mie(r_mie() | MIE_MEIE);
}

int plic_claim()
{
  int hart = r_mhartid();
  int irq = *(uint32_t *)PLIC_MCLAIM(hart);
  return irq;
}

void plic_complete(int irq)
{
  int hart = r_mhartid();
  *(uint32_t *)PLIC_MCOMPLETE(hart) = irq;
}