#include "timer.h"

// a scratch area per CPU for machine-mode timer interrupts.

#define interval 20000000 // cycles; about 2 second in qemu.

void timer_init()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid();
  *(reg_t *)CLINT_MTIMECMP(id) = *(reg_t *)CLINT_MTIME + interval;
}

static int timer_count = 0;

void timer_handler()
{
  lib_printf("timer_handler: %d\n", ++timer_count);
  int id = r_mhartid();
  //lib_printf("hid: %d\n", id);
  *(reg_t *)CLINT_MTIMECMP(id) = *(reg_t *)CLINT_MTIME + interval;
}
