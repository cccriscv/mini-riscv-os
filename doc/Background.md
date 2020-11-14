# 背景知識


file:///D:/ccc109/sp/10-riscv/pdf/RISC-V-Reader-Chinese-v2p1.pdf 

101 頁

有三种标准的中断源：软件、时钟和外部来源。软件中断通过向内存映射寄存器中存数来触发，并通常用于由一个 hart 中断另一个 hart（在其他架构中称为处理器间中断机制）。当 hart 的时间比较器（一个名为 mtimecmp 的内存映射寄存器）大于实时计数器mtime 时，会触发时钟中断。。外部中断由平台级中断控制器（大多数外部设备连接到这个中断控制器）引发。不同的硬件平台具有不同的内存映射并且需要中断控制器的不同特性，因此用于发出和消除这些中断的机制因平台而异。所有 RISC-V 系统的共同问题是如何处理异常和屏蔽中断，这是下一节的主题。

10.3 机器模式下的异常处理

八个控制状态寄存器（CSR）是机器模式下异常处理的必要部分：

* mtvec（Machine Trap Vector）它保存发生异常时处理器需要跳转到的地址。
* mepc（Machine Exception PC）它指向发生异常的指令。
* mcause（Machine Exception Cause）它指示发生异常的种类。
* mie（Machine Interrupt Enable）它指出处理器目前能处理和必须忽略的中断。
* mip（Machine Interrupt Pending）它列出目前正准备处理的中断。
* mtval（Machine Trap Value）它保存了陷入（trap）的附加信息：地址例外中出错的地址、发生非法指令例外的指令本身，对于其他异常，它的值为 0。
* mscratch（Machine Scratch）它暂时存放一个字大小的数据。
* mstatus（Machine Status）它保存全局中断使能，以及许多其他的状态，如图10.4 所示。

处理器在 M 模式下运行时，只有在全局中断使能位 mstatus.MIE 置 1 时才会产生中断.此外，每个中断在控制状态寄存器 mie 中都有自己的使能位。这些位在 mie 中的位置，对应于图 10.3 中的中断代码。例如，mie[7]对应于 M 模式中的时钟中断。控制状态寄存器mip具有相同的布局，并且它指示当前待处理的中断。将所有三个控制状态寄存器合在一起考虑，如果 status.MIE = 1，mie[7] = 1，且 mip[7] = 1，则可以处理机器的时钟中断。

当一个 hart 发生异常时，硬件自动经历如下的状态转换：

* 异常指令的 PC 被保存在 mepc 中，PC 被设置为 mtvec。（对于同步异常，mepc 指向导致异常的指令；对于中断，它指向中断处理后应该恢复执行的位置。）
    * mepc = PC; PC = mtvec
* 根据异常来源设置 mcause（如图 10.3 所示），并将 mtval 设置为出错的地址或 者其它适用于特定异常的信息字。
* 把控制状态寄存器 mstatus 中的 MIE 位置零以禁用中断，并把先前的 MIE 值保留到 MPIE 中。
* 发生异常之前的权限模式保留在 mstatus 的 MPP 域中，再把权限模式更改为M。图 10.5 显示了 MPP 域的编码（如果处理器仅实现 M 模式，则有效地跳过这个步骤）。

为避免覆盖整数寄存器中的内容，中断处理程序先在最开始用 mscratch 和整数寄存器（例如 a0）中的值交换。通常，软件会让 mscratch 包含指向附加临时内存空间的指针，处理程序用该指针来保存其主体中将会用到的整数寄存器。在主体执行之后，中断程序会恢复它保存到内存中的寄存器，然后再次使用 mscratch 和 a0 交换，将两个寄存器恢复到它们在发生异常之前的值。

最后，处理程序用 mret 指令（M 模式特有的指令）返回。mret 将 PC 设置为 mepc，通过将 mstatus 的 MPIE 域复制到 MIE 来恢复之前的中断使能设置，并将权限模式设置为 mstatus 的 MPP 域中的值。这基本是前一段中描述的逆操作。

> mret:  PC=mepc; mstatus:MIE=MPIE;

图 10.6 展示了遵循此模式的基本时钟中断处理程序的 RISC-V 汇编代码。它只对时间比较器执行了递增操作，然后继续执行之前的任务。更实际的时钟中断处理程序可能会调用调度程序，从而在任务之间切换。它是非抢占的，因此在处理程序的过程中中断会被禁用。不考虑这些限制条件的话，它就是一个只有一页的 RISC-V 中断处理程序的完整示例！

![](img/InterruptHandler.png)

图 10.6；简单的 RISC-V 时钟中断处理程序代码。代码中假定了全局中断已通过置位 mstatus.MIE 启
用；时钟中断已通过置位 mie[7]启用；mtvec CSR 已设置为此处理程序的入口地址；而且 mscratch
CSR 已经设置为有 16 个字节用于保存寄存器的临时空间的地址。第一部分保存了五个寄存器，把 a0 保存在 mscratch 中，a1 到 a4 保存在内存中。然后它检查 mcause 来读取异常的类别：如果 mcause<0 则是中断，反之则是同步异常。如果是中断，就检查 mcause 的低位是否等于 7，如果是，就是 M 模式的时钟中断。如果确定是时钟中断，就给时间比较器加上 1000 个时钟周期，于是下一个时钟中断会发生在大约 1000 个时钟周期之后。最后一段恢复了 a0 到 a4 和 mscratch，然后用 mret 指令返回。

默认情况下，发生所有异常（不论在什么权限模式下）的时候，控制权都会被移交到M 模式的异常处理程序。但是 Unix 系统中的大多数例外都应该进行 S 模式下的系统调用。M 模式的异常处理程序可以将异常重新导向 S 模式，但这些额外的操作会减慢大多数异常的处理速度。因此，RISC-V 提供了一种异常委托机制。通过该机制可以选择性地将中断和同步异常交给 S 模式处理，而完全绕过 M 模式。

mideleg（Machine Interrupt Delegation，机器中断委托）CSR 控制将哪些中断委托给 S模式。与 mip 和 mie 一样，mideleg 中的每个位对应于图 10.3 中相同的异常。例如，mideleg[5]对应于 S 模式的时钟中断，如果把它置位，S 模式的时钟中断将会移交 S 模式的异常处理程序，而不是 M 模式的异常处理程序。

委托给 S 模式的任何中断都可以被 S 模式的软件屏蔽。sie（Supervisor InterruptEnable，监管者中断使能）和 sip（Supervisor Interrupt Pending，监管者中断待处理）CSR是 S 模式的控制状态寄存器，他们是 mie 和 mip 的子集。它们有着和 M 模式下相同的布局，但在 sie 和 sip 中只有与由 mideleg 委托的中断对应的位才能读写。那些没有被委派的中断对应的位始终为零。

M 模式还可以通过 medeleg CSR 将同步异常委托给 S 模式。该机制类似于刚才提到的中断委托，但 medeleg 中的位对应的不再是中断，而是图 10.3 中的同步异常编码。例如，置上 medeleg[15]便会把 store page fault（store 过程中出现的缺页）委托给 S 模式。

请注意，无论委派设置是怎样的，发生异常时控制权都不会移交给权限更低的模式。

在 M 模式下发生的异常总是在 M 模式下处理。在 S 模式下发生的异常，根据具体的委派设置，可能由 M 模式或 S 模式处理，但永远不会由 U 模式处理。

S 模式有几个异常处理 CSR：sepc、stvec、scause、sscratch、stval 和 sstatus，它们执行与 10.2 中描述的 M 模式 CSR 相同的功能。图 10.9 显示了 sstatus 寄存器的布局。

监管者异常返回指令 sret 与 mret 的行为相同，但它作用于 S 模式的异常处理 CSR，而不是 M 模式的 CSR。

S 模式处理例外的行为已和 M 模式非常相似。如果 hart 接受了异常并且把它委派给了S 模式，则硬件会原子地经历几个类似的状态转换，其中用到了 S 模式而不是 M 模式的CSR：

* 发生例外的指令的 PC 被存入 sepc，且 PC 被设置为 stvec。
* scause 按图 10.3 根据异常类型设置，stval 被设置成出错的地址或者其它特定异常的信息字。
* 把 sstatus CSR 中的 SIE 置零，屏蔽中断，且 SIE 之前的值被保存在 SPIE 中。
* 发生例外时的权限模式被保存在 sstatus 的 SPP 域，然后设置当前模式为 S 模式。

## 

* https://github.com/RISCV-on-Microsemi-FPGA/RVBM-BootLoader/blob/master/src/riscv_hal/startup.S
* https://github.com/RISCV-on-Microsemi-FPGA/RVBM-BootLoader/blob/master/src/riscv_hal/entry.S
* https://github.com/RISCV-on-Microsemi-FPGA/RVBM-BootLoader/blob/master/src/interrupts.c

```cpp
/******************************************************************************
 * RISC-V interrupt handler for machine timer interrupts.
 *****************************************************************************/
void handle_m_timer_interrupt(){

    clear_csr(mie, MIP_MTIP);

    add_10ms_to_mtimecmp();

    SysTick_Handler();

    // Re-enable the timer interrupt.
    set_csr(mie, MIP_MTIP);
}
```

```cpp
/*------------------------------------------------------------------------------
 * Count the number of elapsed milliseconds (SysTick_Handler is called every
 * 10mS so the resolution will be 10ms). Rolls over every 49 days or so...
 *
 * Should be safe to read g_10ms_count from elsewhere.
 */
void SysTick_Handler(void)
{
    g_10ms_count += 10;

     /*
      * For neatness, if we roll over, reset cleanly back to 0 so the count
      * always goes up in proper 10s.
      */
    if(g_10ms_count < 10)
        g_10ms_count = 0;
}
```

```cpp
uintptr_t handle_trap(uintptr_t mcause, uintptr_t epc)
{
  if (0){
  // External Machine-Level Interrupt from PLIC
  }else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_EXT)) {
    handle_m_ext_interrupt();
  }else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE)  == IRQ_M_TIMER)) {
    handle_m_timer_interrupt();
  }    
  else{
    write(1, "trap\n", 5);
    _exit(1 + mcause);
  }
  return epc;
}
```

start.s

```s
trap_entry:
  addi sp, sp, -32*REGBYTES
...
  csrr a0, mcause
  csrr a1, mepc
  mv a2, sp
  jal handle_trap
  csrw mepc, a0
  # Remain in M-mode after mret
  li t0, MSTATUS_MPP
  csrs mstatus, t0
...
  addi sp, sp, 32*REGBYTES
  mret
```

* https://github.com/d0iasm/rvemu/blob/master/src/cpu.rs

```rust
// mret
// "The RISC-V Reader" book says:
// "Returns from a machine-mode exception handler. Sets the pc to
// CSRs[mepc], the privilege mode to CSRs[mstatus].MPP,
// CSRs[mstatus].MIE to CSRs[mstatus].MPIE, and CSRs[mstatus].MPIE
// to 1; and, if user mode is supported, sets CSRs[mstatus].MPP to 0".
```

* RISC-V-Reader-Chinese-v2p1.pdf

* mret ExceptionReturn(Machine)
	* 机器模式异常返回(Machine-mode Exception Return). R-type, RV32I and RV64I 特权架构从机器模式异常处理程序返回。将 pc 设置为 CSRs[mepc], 将特权级设置成 CSRs[mstatus].MPP, CSRs[mstatus].MIE 置成 CSRs[mstatus].MPIE, 并且将 CSRs[mstatus].MPIE 为 1;并且，如果支持用户模式，则将 CSR [mstatus].MPP 设置为 0。

```cpp
  // set the machine-mode trap handler.
  w_mtvec((reg_t)sys_timer);
```

```cpp
// Machine-mode interrupt vector
static inline void w_mtvec(reg_t x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}
```

