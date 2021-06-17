# 01-HelloOs -- 嵌入式 RISC-V 輸出入程式

專案 -- https://github.com/ccc-c/mini-riscv-os/tree/master/01-HelloOs

在本系列的文章中，我們將介紹如何建構一個 RISC-V 處理器上的嵌入式作業系統，該作業系統名稱為 mini-riscv-os 。(其實是一系列的程式，非單一系統)

首先在本章中，我們將介紹一個最簡單可以印出 `Hello OS!` 的程式該怎麼寫！

## os.c

https://github.com/ccc-c/mini-riscv-os/blob/master/01-HelloOs/os.c

```cpp
#include <stdint.h>

#define UART        0x10000000
#define UART_THR    (uint8_t*)(UART+0x00) // THR:transmitter holding register
#define UART_LSR    (uint8_t*)(UART+0x05) // LSR:line status register
#define UART_LSR_EMPTY_MASK 0x40          // LSR Bit 6: Transmitter empty; both the THR and LSR are empty

int lib_putc(char ch) {
	while ((*UART_LSR & UART_LSR_EMPTY_MASK) == 0);
	return *UART_THR = ch;
}

void lib_puts(char *s) {
	while (*s) lib_putc(*s++);
}

int os_main(void)
{
	lib_puts("Hello OS!\n");
	while (1) {}
	return 0;
}
```

QEMU 中預設的 RISC-V 虛擬機稱為 virt ，其中的 UART 記憶體映射位置從 0x10000000 開始，映射方式如下所示：

```
UART 映射區

0x10000000 THR (Transmitter Holding Register) 同時也是 RHR (Receive Holding Register)
0x10000001 IER (Interrupt Enable Register)
0x10000002 ISR (Interrupt Status Register)
0x10000003 LCR (Line Control Register)
0x10000004 MCR (Modem Control Register)
0x10000005 LSR (Line Status Register)
0x10000006 MSR (Modem Status Register)
0x10000007 SPR (Scratch Pad Register)
```

我們只要將某字元往 UART 的 THR 送，就可以印出該字元，但送之前必須確認 LSR 的第六位元是否為 1 (代表 UART 傳送區為空，可以傳送了)。

```
THR Bit 6: Transmitter empty; both the THR and shift register are empty if this is set.
```

所以我們寫了下列函數來送一個字元給 UART 以便印出到宿主機 (host) 上。(因為嵌入式系統通常沒有顯示裝置，所以會送回宿主機顯示)

```cpp
int lib_putc(char ch) {
	while ((*UART_LSR & UART_LSR_EMPTY_MASK) == 0);
	return *UART_THR = ch;
}
```

能印出一個字之後，就能用以下的 lib_puts(s) 印出一大串字。

```cpp
void lib_puts(char *s) {
	while (*s) lib_putc(*s++);
}
```

於是我們的主程式就呼叫 lib_puts 印出了 `Hello OS!` 。

```cpp
int os_main(void)
{
	lib_puts("Hello OS!\n");
	while (1) {}
	return 0;
}
```

雖然我們的主程式只有短短的 22 行，但是 01-HelloOs 專案包含的不是只有主程式，還有啟動程式 start.s ，連結檔案 os.ld，以及建置檔 Makefile。

## 專案建置檔 Makefile

mini-riscv-os 裏的 Makefile 通常大同小異，以下是 01-HelloOs 的 Makefile.

```Makefile
CC = riscv64-unknown-elf-gcc
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32

QEMU = qemu-system-riscv32
QFLAGS = -nographic -smp 4 -machine virt -bios none

OBJDUMP = riscv64-unknown-elf-objdump

all: os.elf

os.elf: start.s os.c
	$(CC) $(CFLAGS) -T os.ld -o os.elf $^

qemu: $(TARGET)
	@qemu-system-riscv32 -M ? | grep virt >/dev/null || exit
	@echo "Press Ctrl-A and then X to exit QEMU"
	$(QEMU) $(QFLAGS) -kernel os.elf

clean:
	rm -f *.elf
```

Makefile 的有些語法不容易懂，特別是下列的符號：

```
$@ : 該規則的目標文件 (Target file)
$* : 代表 targets 所指定的檔案，但不包含副檔名
$< : 依賴文件列表中的第一個依賴文件 (Dependencies file)
$^ : 依賴文件列表中的所有依賴文件
$? : 依賴文件列表中新於目標文件的文件列表
$* : 代表 targets 所指定的檔案，但不包含副檔名

?= 語法 : 若變數未定義，則替它指定新的值。
:= 語法 : make 會將整個 Makefile 展開後，再決定變數的值。
```

所以上述 Makefile 中的下列兩行：

```Makefile
os.elf: start.s os.c
	$(CC) $(CFLAGS) -T os.ld -o os.elf $^
```

其中的 `$^` 被代換成 `start.s os.c` ，於是整個 `$(CC) $(CFLAGS) -T os.ld -o os.elf $^` 整行展開後就變成了下列指令。

```
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s os.c
```

在 Makefile 中我們使用 riscv64-unknown-elf-gcc 去編譯，然後用 qemu-system-riscv32 去執行， 01-HelloOs 的執行過程如下：

```
user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/01-HelloOs (master)
$ make clean
rm -f *.elf

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/01-HelloOs (master)
$ make
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s os.c

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/01-HelloOs (master)
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
Hello OS!
QEMU: Terminated
```

首先用 make clean 清除上次的編譯產出，然後用 make 呼叫 riscv64-unknown-elf-gcc 編譯專案，以下是完整的編譯指令

```
$ riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s os.c
```

其中 `-march=rv32ima` 代表我們要 [產生 32 位元 I+M+A 指令集的碼](https://www.sifive.com/blog/all-aboard-part-1-compiler-args)：

```
I: 基本整數指令集 (Integer)
M: 包含乘除法 (Multiply)
A: 包含原子指令 (Atomic)
C: 使用 16 位元壓縮 (Compact) -- 注意：我們沒加 C ，所以產生的指令機器碼是純粹 32 位元指令，沒有被壓成 16 位元，因為我們希望指令長度大小一致，從頭到尾都是 32 bits。
```

而 `-mabi=ilp32` 代表產生的二進位目的碼的整數是以 32 位元為主的架構。

- ilp32: int, long, and pointers are all 32-bits long. long long is a 64-bit type, char is 8-bit, and short is 16-bit.
- lp64: long and pointers are 64-bits long, while int is a 32-bit type. The other types remain the same as ilp32.

還有 `-mcmodel=medany` 參數代表產生符號位址必須可在 2GB 內，可以使用靜態連結的方式定址。

- `-mcmodel=medany`
    * Generate code for the medium-any code model. The program and its statically defined symbols must be within any single 2 GiB address range. Programs can be statically or dynamically linked.

更詳細的 RISC-V gcc 參數可以參考下列文件：

* https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html

另外還使用了 `-nostdlib -fno-builtin` 這兩個參數代表不去連結使用標準函式庫 (因為是嵌入式系統，函式庫通常得自製)，請參考下列文件：

* https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html


## Link Script 連結檔 (os.ld)

還有 `-T os.ld` 參數指定了 link script 為 os.ld 檔案如下：(link script 是描述程式段 TEXT、資料段 DATA 與 BSS 未初始化資料段分別該如何放入記憶體的指引檔)

```ld
OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
  ram   (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

PHDRS
{
  text PT_LOAD;
  data PT_LOAD;
  bss PT_LOAD;
}

SECTIONS
{
  .text : {
    PROVIDE(_text_start = .);
    *(.text.init) *(.text .text.*)
    PROVIDE(_text_end = .);
  } >ram AT>ram :text

  .rodata : {
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >ram AT>ram :text

  .data : {
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*) *(.data .data.*)
    PROVIDE(_data_end = .);
  } >ram AT>ram :data

  .bss :{
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    PROVIDE(_bss_end = .);
  } >ram AT>ram :bss

  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
}
```

## 啟動程式 (start.s)

嵌入式系統除了主程式外，通常還需要一個組合語言寫的啟動程式，01-HelloOs 裏的啟動程式 start.s 內容如下：(主要是讓多核心的架構下，只有一個核心被啟動，其他核心都睡著，這樣可以讓事情變得更單純，不需考慮太多平行處理方面的問題)。
 
```s
.equ STACK_SIZE, 8192

.global _start

_start:
    # setup stacks per hart
    csrr t0, mhartid                # read current hart id
    slli t0, t0, 10                 # shift left the hart id by 1024
    la   sp, stacks + STACK_SIZE    # set the initial stack pointer 
                                    # to the end of the stack space
    add  sp, sp, t0                 # move the current hart stack pointer
                                    # to its place in the stack space

    # park harts with id != 0
    csrr a0, mhartid                # read current hart id
    bnez a0, park                   # if we're not on the hart 0
                                    # we park the hart

    j    os_main                    # hart 0 jump to c

park:
    wfi
    j park

stacks:
    .skip STACK_SIZE * 4            # allocate space for the harts stacks

```

## 用 QEMU 執行

而當你打入 make qemu 時，Make 會執行下列指令

```
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
```

代表要用 qemu-system-riscv32 執行 os.elf 這個 kernel 檔案，`-bios none` 不使用基本輸出入 bios，`-nographic` 不使用繪圖模式 ，而指定的機器架構為 `-machine virt` ，也就是 QEMU 預設的 RISC-V 虛擬機 virt。

於是當你打入 `make qemu` 時，就會看到下列畫面了！

```
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
Hello OS!
QEMU: Terminated
```

這就是 RISC-V 嵌入式系統裏一個最簡單的 Hello 程式的基本樣貌了。
