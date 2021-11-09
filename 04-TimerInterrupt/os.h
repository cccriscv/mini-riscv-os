#ifndef __OS_H__
#define __OS_H__

#include "riscv.h"
#include "lib.h"
#include "timer.h"

extern void os_loop(void);
extern int  os_main(void);

#endif
