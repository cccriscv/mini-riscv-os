#ifndef __RISCV_H__
#define __RISCV_H__

#include <stdint.h>

#define reg_t uint32_t // RISCV32: register is 32bits
// define reg_t as uint64_t // RISCV64: register is 64bits

// ref: https://www.activexperts.com/serial-port-component/tutorials/uart/
#define UART        0x10000000
#define UART_THR    (uint8_t*)(UART+0x00) // THR:transmitter holding register
#define UART_LSR    (uint8_t*)(UART+0x05) // LSR:line status register
#define UART_LSR_EMPTY_MASK 0x40          // LSR Bit 6: Transmitter empty; both the THR and LSR are empty

// Saved registers for kernel context switches.
struct context {
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

#endif
