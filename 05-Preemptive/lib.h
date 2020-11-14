#ifndef __LIB_H__
#define __LIB_H__

#include "riscv.h"
#include <stddef.h>
#include <stdarg.h>

#define lib_error(...) { lib_printf(__VA_ARGS__); while(1) {} } }

extern void lib_delay(volatile int count);
extern int  lib_putc(char ch);
extern void lib_puts(char *s);
extern int  lib_printf(const char* s, ...);
extern int  lib_vprintf(const char* s, va_list vl);
extern int  lib_vsnprintf(char * out, size_t n, const char* s, va_list vl);

#endif
