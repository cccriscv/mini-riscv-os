#ifndef __SYS_H__
#define __SYS_H__

#include "riscv.h"
extern void sys_timer();
extern void sys_switch(struct context *ctx_old, struct context *ctx_new);
extern void switch_to(struct context *ctx_next);

#endif
