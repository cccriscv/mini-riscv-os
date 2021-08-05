# 07-ExterInterrupt -- RISC-V 的嵌入式作業系統

## 先備知識

### PIC

PIC (Programmable Interrupt Controller) 是一個特殊用途的電路，可以幫助處理器處理不同來源的(同時)發生的中斷請求。它有助於確定 IRQ 的優先級，讓 CPU 執行切換到最合適的中斷處理程序。

### Interrupt

先重新複習 RISC-V 的中斷種類，可細分為幾大項:

- Local Interrupt
  - Software Interrupt
  - Timer Interrupt
- Global Interrupt
  - External Interrupt

各種中斷的 Exception Code 也都有被規格書詳細的定義:

![](https://camo.githubusercontent.com/9f3e34c3f929a4b693a1c198586e0a67c78f8e3d42773fafde0746355196030d/68747470733a2f2f692e696d6775722e636f6d2f7a6d756b6e51722e706e67)

> Exception code 會被紀錄在 mcause 暫存器當中。

若我們要讓運行在 RISC-V 中的系統程式支援中斷處理，也需要設定 MIE Register 的域值:

```cpp
// Machine-mode Interrupt Enable
#define MIE_MEIE (1 << 11) // external
#define MIE_MTIE (1 << 7)  // timer
#define MIE_MSIE (1 << 3)  // software
// enable machine-mode timer interrupts.
w_mie(r_mie() | MIE_MTIE);
```

### PLIC

大致複習了先前介紹的中斷處理後，讓我們回到本文的重點: **PLIC** 來看。
PLIC (Platform-Level Interrupt Controller) 就是為了 RISC-V 平台所打造的 PIC 。
實際上，會有多個中斷源(鍵盤、滑鼠、硬碟...)接上 PLIC ， PLIC 會判別這些中斷的優先級，再分配給處理器的 Hart (RISC-V 中 hardware thread 的最小單位) 進行中斷處理。

### IRQ

> 在電腦科學中，中斷是指處理器接收到來自硬體或軟體的訊號，提示發生了某個事件，應該被注意，這種情況就稱為中斷。 通常，在接收到來自外圍硬體的非同步訊號，或來自軟體的同步訊號之後，處理器將會進行相應的硬體／軟體處理。發出這樣的訊號稱為進行中斷請求 (IRQ) 。 -- wikipedia

以 Qemu 中的 RISC-V 虛擬機器 - Virt 為例，它的[原始碼](https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h)就定義了不同中斷的 IRQ :

```cpp
enum {
    UART0_IRQ = 10,
    RTC_IRQ = 11,
    VIRTIO_IRQ = 1, /* 1 to 8 */
    VIRTIO_COUNT = 8,
    PCIE_IRQ = 0x20, /* 32 to 35 */
    VIRTIO_NDEV = 0x35 /* Arbitrary maximum number of interrupts */
};
```

當我們在撰寫作業系統時，就可以利用 IRQ 的代號去判別外部中斷的類型，達成鍵盤輸入、磁碟讀寫的問題，關於這些內容，筆者會在之後的文章做更深入的介紹。

### PLIC 的 Memory Map

至於我們到底該如何與 PLIC 進行溝通呢?
PLIC 是採取 Memory Map 的機制，它會將一些重要的資訊映射到 Main Memory 當中，如此一來，我們就可以透過存取記憶體的方式做到與 PLIC 的溝通。
我們可以繼續看到 [Virt 的原始碼](https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c) ，它定義了 PLIC 的虛擬位置:

```cpp
static const MemMapEntry virt_memmap[] = {
    [VIRT_DEBUG] =       {        0x0,         0x100 },
    [VIRT_MROM] =        {     0x1000,        0xf000 },
    [VIRT_TEST] =        {   0x100000,        0x1000 },
    [VIRT_RTC] =         {   0x101000,        0x1000 },
    [VIRT_CLINT] =       {  0x2000000,       0x10000 },
    [VIRT_PCIE_PIO] =    {  0x3000000,       0x10000 },
    [VIRT_PLIC] =        {  0xc000000, VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2) },
    [VIRT_UART0] =       { 0x10000000,         0x100 },
    [VIRT_VIRTIO] =      { 0x10001000,        0x1000 },
    [VIRT_FW_CFG] =      { 0x10100000,          0x18 },
    [VIRT_FLASH] =       { 0x20000000,     0x4000000 },
    [VIRT_PCIE_ECAM] =   { 0x30000000,    0x10000000 },
    [VIRT_PCIE_MMIO] =   { 0x40000000,    0x40000000 },
    [VIRT_DRAM] =        { 0x80000000,           0x0 },
};
```

每一個 PLIC 的中斷源都會由一個暫存器作為代表，將 `PLIC_BASE` 加上暫存器的偏移量 `offset`我們就可以知道暫存器映射到主記憶體的位置。

```
0xc000000 (PLIC_BASE) + offset = Mapped Address of register
```

## 進入正題: 使 mini-riscv-os 支援外部中斷

首先，看到 `plic_init()` ，該檔案定義在 `plic.c`:

```cpp
void plic_init()
{
  int hart = r_tp();
  // QEMU Virt machine support 7 priority (1 - 7),
  // The "0" is reserved, and the lowest priority is "1".
  *(uint32_t *)PLIC_PRIORITY(UART0_IRQ) = 1;

  /* Enable UART0 */
  *(uint32_t *)PLIC_MENABLE(hart) = (1 << UART0_IRQ);

  /* Set priority threshold for UART0. */

  *(uint32_t *)PLIC_MTHRESHOLD(hart) = 0;

  /* enable machine-mode external interrupts. */
  w_mie(r_mie() | MIE_MEIE);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);
}
```

看到上面的範例， `plic_init()` 主要做了這些初始化動作:

- 設定 UART_IRQ 的優先權
  因為 PLIC 可以管理多個外部中斷源，我們必須為不同的中斷源設定優先順序，當這些中斷源衝突時， PLIC 才會知道要先處理哪個 IRQ 。
- 針對 hart0 開啟 UART 中斷
- 設定 threshold
  小於或是等於這個閥值的 IRQ 會被 PLIC 無視，如果我們將範例改成:

```cpp
*(uint32_t *)PLIC_MTHRESHOLD(hart) = 10;
```

這樣系統就不會處理 UART 的 IRQ 了。

- 開啟外部中斷與 Machine mode 下的全局中斷
  需要注意的是，本專案原先是在 `trap_init()` 開啟 Machine mode 下的全局中斷，在這次的修改後，我們改讓 `plic_init()` 負責。

> 除了 PLIC 需要做初始化以外，還有 UART 需要做初始化設定，像是設定 **baud rate** 等動作， `uart_init()` 定義在 `lib.c` 中，有興趣的讀者可以自行查閱。

### 修改 Trap Handler

```
                         +----------------+
                         | soft_handler() |
                 +-------+----------------+
                 |
+----------------+-------+-----------------+
| trap_handler() |       | timer_handler() |
+----------------+       +-----------------+
                 |
                 +-------+-----------------+
                         | exter_handler() |
                         +-----------------+
```

先前 `trap_handler()` 只有支援時間中斷的處理，這次我們則是要讓它支援外部中斷的處理:

```cpp
/* In trap.c */
void external_handler()
{
  int irq = plic_claim();
  if (irq == UART0_IRQ)
  {
    lib_isr();
  }
  else if (irq)
  {
    lib_printf("unexpected interrupt irq = %d\n", irq);
  }

  if (irq)
  {
    plic_complete(irq);
  }
}
```

因為本次的目標是讓作業系統能夠處理 UART IRQ ，所以透過上面的程式碼不難發現我們只對 UART 做處理:

```cpp
/* In lib.c */
void lib_isr(void)
{
    for (;;)
    {
        int c = lib_getc();
        if (c == -1)
        {
            break;
        }
        else
        {
            lib_putc((char)c);
            lib_putc('\n');
        }
    }
}
```

`lib_isr()` 的原理也相當簡單，只是重複的偵測 UART 的 RHR 暫存器有沒有收到新的資料，如果為空 (c == -1) 則跳出迴圈。

> 與 UART 有關的暫存器都定義在 `riscv.h` 之中，這次為了支援 `lib_getc()` 添加了一些暫存器位址，大致內容如下:
>
> ```cpp
> #define UART 0x10000000L
> #define UART_THR (volatile uint8_t *)(UART + 0x00) // THR:transmitter holding register
> #define UART_RHR (volatile uint8_t *)(UART + 0x00) // RHR:Receive holding register
> #define UART_DLL (volatile uint8_t *)(UART + 0x00) // LSB of Divisor Latch (write mode)
> #define UART_DLM (volatile uint8_t *)(UART + 0x01) // MSB of Divisor Latch (write mode)
> #define UART_IER (volatile uint8_t *)(UART + 0x01) // Interrupt Enable Register
> #define UART_LCR (volatile uint8_t *)(UART + 0x03) // Line Control Register
> #define UART_LSR (volatile uint8_t *)(UART + 0x05) // LSR:line status register
> #define UART_LSR_EMPTY_MASK 0x40                   // LSR Bit 6: Transmitter empty; both the THR and LSR are empty
> ```

本次提交的修改內容大致如上，其中還有一些實作細節沒有特別提出，建議有興趣的讀者可以直接 Trace 原始碼，相信會更有收穫。
有了這些基礎，之後可以添加像是:

- virtio driver & file system
- system call
- mini shell
  等功能，讓 `mini-riscv-os` 更具規模。

## Reference

- [Step by step, learn to develop an operating system on RISC-V](https://github.com/plctlab/riscv-operating-system-mooc)
- [Qemu](https://github.com/qemu/qemu)
- [AwesomeCS wiki](https://github.com/ianchen0119/AwesomeCS/wiki/2-5-RISC-V::%E4%B8%AD%E6%96%B7%E8%88%87%E7%95%B0%E5%B8%B8%E8%99%95%E7%90%86----PLIC-%E4%BB%8B%E7%B4%B9)
