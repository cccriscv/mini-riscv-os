#ifndef __RISCV_H__
#define __RISCV_H__

#include <stdint.h>

#define reg_t uint32_t // RISCV32: register is 32bits
// define reg_t as uint64_t // RISCV64: register is 64bits

// ref: https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h
// Intro: https://github.com/ianchen0119/AwesomeCS/wiki/2-5-RISC-V::%E4%B8%AD%E6%96%B7%E8%88%87%E7%95%B0%E5%B8%B8%E8%99%95%E7%90%86----PLIC-%E4%BB%8B%E7%B4%B9
#define PLIC_BASE 0x0c000000L
#define PLIC_PRIORITY(id) (PLIC_BASE + (id)*4)
#define PLIC_PENDING(id) (PLIC_BASE + 0x1000 + ((id) / 32))
#define PLIC_MENABLE(hart) (PLIC_BASE + 0x2000 + (hart)*0x80)
#define PLIC_MTHRESHOLD(hart) (PLIC_BASE + 0x200000 + (hart)*0x1000)
#define PLIC_MCLAIM(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)
#define PLIC_MCOMPLETE(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)

// ref: https://www.activexperts.com/serial-port-component/tutorials/uart/
#define UART 0x10000000L
#define UART_THR (uint8_t *)(UART + 0x00) // THR:transmitter holding register
#define UART_RHR (uint8_t *)(UART + 0x00) // RHR:Receive holding register
#define UART_DLL (uint8_t *)(UART + 0x00) // LSB of Divisor Latch (write mode)
#define UART_DLM (uint8_t *)(UART + 0x01) // MSB of Divisor Latch (write mode)
#define UART_IER (uint8_t *)(UART + 0x01) // Interrupt Enable Register
#define UART_LCR (uint8_t *)(UART + 0x03) // Line Control Register
#define UART_LSR (uint8_t *)(UART + 0x05) // LSR:line status register
#define UART_LSR_EMPTY_MASK 0x40          // LSR Bit 6: Transmitter empty; both the THR and LSR are empty

#define UART_REGR(reg) (*(reg))
#define UART_REGW(reg, v) ((*reg) = (v))

// ref: https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h
#define UART0_IRQ 10
#define VIRTIO_IRQ 1

// Saved registers for kernel context switches.
struct context
{
  reg_t ra;
  reg_t sp;

  // callee-saved
  reg_t s0;
  reg_t s1;
  reg_t s2;
  reg_t s3;
  reg_t s4;
  reg_t s5;
  reg_t s6;
  reg_t s7;
  reg_t s8;
  reg_t s9;
  reg_t s10;
  reg_t s11;
};

// ref: https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/riscv.h
//
// local interrupt controller, which contains the timer.
// ================== Timer Interrput ====================

#define NCPU 8 // maximum number of CPUs
#define CLINT 0x2000000
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 4 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

static inline reg_t r_tp()
{
  reg_t x;
  asm volatile("mv %0, tp"
               : "=r"(x));
  return x;
}

// which hart (core) is this?

static inline reg_t r_mhartid()
{
  reg_t x;
  asm volatile("csrr %0, mhartid"
               : "=r"(x));
  return x;
}

// Machine Status Register, mstatus
#define MSTATUS_MPP_MASK (3 << 11) // previous mode.
#define MSTATUS_MPP_M (3 << 11)
#define MSTATUS_MPP_S (1 << 11)
#define MSTATUS_MPP_U (0 << 11)
#define MSTATUS_MIE (1 << 3) // machine-mode interrupt enable.

static inline reg_t r_mstatus()
{
  reg_t x;
  asm volatile("csrr %0, mstatus"
               : "=r"(x));
  return x;
}

static inline void w_mstatus(reg_t x)
{
  asm volatile("csrw mstatus, %0"
               :
               : "r"(x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_mepc(reg_t x)
{
  asm volatile("csrw mepc, %0"
               :
               : "r"(x));
}

static inline reg_t r_mepc()
{
  reg_t x;
  asm volatile("csrr %0, mepc"
               : "=r"(x));
  return x;
}

// Machine Scratch register, for early trap handler
static inline void w_mscratch(reg_t x)
{
  asm volatile("csrw mscratch, %0"
               :
               : "r"(x));
}

// Machine-mode interrupt vector
static inline void w_mtvec(reg_t x)
{
  asm volatile("csrw mtvec, %0"
               :
               : "r"(x));
}

// Machine-mode Interrupt Enable
#define MIE_MEIE (1 << 11) // external
#define MIE_MTIE (1 << 7)  // timer
#define MIE_MSIE (1 << 3)  // software

static inline reg_t r_mie()
{
  reg_t x;
  asm volatile("csrr %0, mie"
               : "=r"(x));
  return x;
}

static inline void w_mie(reg_t x)
{
  asm volatile("csrw mie, %0"
               :
               : "r"(x));
}

#endif
