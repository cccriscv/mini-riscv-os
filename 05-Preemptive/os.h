#ifndef __OS_H__
#define __OS_H__

#include "riscv.h"
#include "lib.h"
#include "task.h"
#include "timer.h"

extern void user_init();
extern void os_kernel();
extern int  os_main(void);

#endif
