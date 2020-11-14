#include "lib.h"

void lib_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

int lib_putc(char ch) {
	while ((*UART_LSR & UART_LSR_EMPTY_MASK) == 0);
	return *UART_THR = ch;
}

void lib_puts(char *s) {
	while (*s) lib_putc(*s++);
}
