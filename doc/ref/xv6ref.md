# xv6 參考

当 xv6 内核在 CPU 上执行时，可能会发生两种类型的陷阱：异常和设备中断。上一节概述了 CPU 对此类陷阱的响应。

当内核执行时，它指向汇编代码 kernelvec (kernel/kernelve.S：10)。由于xv6已经在内核中，因此kernelvec可以依赖于将`satp`设置为内核页表，并依赖于引用有效内核堆栈的堆栈指针。kernelvec保存所有寄存器，这样我们最终可以恢复中断的代码，而不会干扰它。


***
kernelvec将寄存器保存在中断的内核线程的堆栈上，这是有意义的，因为寄存器值属于该线程。如果陷阱导致切换到不同的线程，这一点尤其重要 - 在这种情况下，陷阱实际上会返回到新线程的堆栈上，将被中断线程的已保存寄存器安全地留在其堆栈上。
***

kernelvec在保存寄存器后跳转到kerneltrap(kernel/trap.c：134)。kerneltrap为两种类型的陷阱做好准备：设备中断和异常。它调用sdevintr(kernel/trap.c：177)来检查和处理前者。如果陷阱不是设备中断，那么它就是一个异常，如果它发生在内核中，那总是一个致命的错误。

## supervisor mode & mret

File: start.c

```cpp
// entry.S jumps here in machine mode on stack0.
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  w_mepc((uint64)main);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

```

File: main.c

```cpp
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode cache
    fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
```