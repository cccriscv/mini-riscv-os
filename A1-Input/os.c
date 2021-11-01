#include <stdint.h>

#define UART        0x10000000            // RISC-V 的 virt 虛擬機，其 UART 映射區起始位址為 0x10000000
#define UART_THR    (uint8_t*)(UART+0x00) // THR:transmitter holding register 傳送暫存器
#define UART_RHR    (uint8_t*)(UART+0x00) // THR:transmitter holding register 傳送暫存器
#define UART_LSR    (uint8_t*)(UART+0x05) // LSR:line status register 輸出狀態暫存器
#define UART_LSR_RX_READY 0x01            // input is waiting to be read from RHR
#define UART_LSR_EMPTY_MASK 0x40          // LSR Bit 6: 當 LSR 的第六位元為 1 時，代表傳送區為空的，可以傳了 (Transmitter empty; both the THR and LSR are empty)

void lib_putc(char ch) { // 透過 UART 傳送字元 ch 給宿主機印出
	while ((*UART_LSR & UART_LSR_EMPTY_MASK) == 0); // 一直等到 UART LSR 傳送區為空，可以傳送了 
	*UART_THR = ch; // 將字元 ch 發送出去
}

void lib_puts(char *s) { // 印出字串到宿主機。
	while (*s) lib_putc(*s++);
}

char lib_getc() {
	while ((*UART_LSR & UART_LSR_RX_READY) == 0); // 一直等到 UART RX_READY 有收到資料 
	return *UART_RHR; // 傳回收到的字元
}

int lib_gets(char *s) {
	char *p = s;
	while (1) {
		char ch = lib_getc();
		lib_putc(ch);
		if (ch == '\n' || ch == '\r') break;
		*p++ = ch;
	}
	*p = '\0';
	return p-s;
}

int os_main(void)
{
	lib_puts("type something:");
	char str[100];
	lib_gets(str);
	lib_puts("\r\nyou type:");
	lib_puts(str);
	lib_puts("\r\n");
	while (1) {} // 讓主程式卡在這裡，用無窮迴圈停止！
	return 0;
}
