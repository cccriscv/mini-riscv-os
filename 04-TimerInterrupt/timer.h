#ifndef __TIMER_H__
#define __TIMER_H__

#include "riscv.h"
#include "sys.h"
#include "lib.h"

extern reg_t timer_handler(reg_t epc, reg_t cause);
extern void timer_init();

#endif
