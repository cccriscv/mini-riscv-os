#ifndef __OS_H__
#define __OS_H__

#include "riscv.h"
#include "lib.h"
#include "task.h"
#include "timer.h"
#include "string.h"

extern void panic(char *);
extern void user_init();
extern void os_kernel();
extern int os_main(void);

// PLIC
extern void plic_init();
extern int plic_claim();
extern void plic_complete(int);

// lock
extern void basic_lock();
extern void basic_unlock();

typedef struct lock
{
  volatile int locked;
} lock_t;

extern int atomic_swap(lock_t *);

extern void lock_init(lock_t *lock);

extern void lock_acquire(lock_t *lock);

extern void lock_free(lock_t *lock);

// memory allocator

extern void *malloc(size_t size);
extern void free(void *p);

#endif
