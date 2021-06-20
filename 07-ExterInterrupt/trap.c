#include "os.h"
extern void trap_vector();

void trap_init()
{
  // set the machine-mode trap handler.
  w_mtvec((reg_t)trap_vector);
}

void external_handler()
{
  int irq = plic_claim();
  if (irq == UART0_IRQ)
  {
    lib_isr();
  }
  else if (irq)
  {
    lib_printf("unexpected interrupt irq = %d\n", irq);
  }

  if (irq)
  {
    plic_complete(irq);
  }
}

reg_t trap_handler(reg_t epc, reg_t cause)
{
  reg_t return_pc = epc;
  reg_t cause_code = cause & 0xfff;

  if (cause & 0x80000000)
  {
    /* Asynchronous trap - interrupt */
    switch (cause_code)
    {
    case 3:
      lib_puts("software interruption!\n");
      break;
    case 7:
      lib_puts("timer interruption!\n");
      // disable machine-mode timer interrupts.
      w_mie(r_mie() & ~(1 << 7));
      timer_handler();
      return_pc = (reg_t)&os_kernel;
      // enable machine-mode timer interrupts.
      w_mie(r_mie() | MIE_MTIE);
      break;
    case 11:
      lib_puts("external interruption!\n");
      external_handler();
      break;
    default:
      lib_puts("unknown async exception!\n");
      break;
    }
  }
  else
  {
    /* Synchronous trap - exception */
    lib_puts("Sync exceptions!\n");
    while (1)
    {
      /* code */
    }
  }
  return return_pc;
}
