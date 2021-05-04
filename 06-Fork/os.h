#ifndef __OS_H__
#define __OS_H__

#include "riscv.h"
#include "lib.h"
#include "task.h"
#include "timer.h"
#include "unistd.h"

extern void user_init();
extern void os_kernel();
void minus_current_task();
extern int os_main(void);
extern int current_task;
extern int get_current_task();

#endif
