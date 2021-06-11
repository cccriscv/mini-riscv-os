# 04-TimerInterrupt -- RISC-V 的時間中斷

[os.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/04-TimerInterrupt/os.c

[timer.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/04-TimerInterrupt/timer.c

[sys.s]:https://github.com/ccc-c/mini-riscv-os/blob/master/04-TimerInterrupt/sys.s

專案 -- https://github.com/ccc-c/mini-riscv-os/tree/master/04-TimerInterrupt

在前一章的 [03-MultiTasking](03-MultiTasking.md) 中我們實作了一個《協同式多工》作業系統。不過由於沒有引入時間中斷機制，無法成為一個《搶先式》(Preemptive) 多工系統。

本章將為第五章的《搶先式多工系統》鋪路，介紹如何在 RISC-V 處理器中使用《時間中斷機制》。有了時間中斷之後，我們就可以定時強制取回控制權，而不用害怕惡霸行程佔據系統，不歸還控制權給作業系統了。

## 先備知識
在學習系統如何實現時間中斷機制之前，我們必須先了解幾件事情:
- 如何產生 Timer interrupt
- 什麼是中斷向量表?
- CSR 暫存器
### 如何產生 Timer interrupt
RISC-V 架構有規定，系統平台必須要有一個計時器。並且，該計時器必須具備兩個 64-bit 的暫存器 mtime 以及 mtimecmp ，前者用於紀錄當前計數器的值，後者則是 mtime 的比較值，當 value of mtime > value of mtimecmp 時便會產生中斷。
而這兩個寄存器也被定義在 [riscv.h](https://github.com/cccriscv/mini-riscv-os/blob/master/04-TimerInterrupt/riscv.h) 中:
```c=
// ================== Timer Interrput ====================

#define NCPU 8             // maximum number of CPUs
#define CLINT 0x2000000
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 4*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.
```
在了解如何產生 Timer interrupt 後，等等在下面的介紹我們就會看到有一段程式碼描述每個中斷觸發的時間間隔 (Interval)。
不只如此，我們還需要在系統初始化的時候開啟 Timer interrupt ，具體的作法是: 開啟將 mie register 負責管理 Timer interrupt 的域寫成 1 。

### 什麼是中斷向量表
中斷向量表是一個由系統程式維護的 Table ，我們可以將對應的 Interrupt_Handler 放進中斷向量表，這樣一來，當時間中斷發生時，系統就會 Trap 進 Interrupt_Handler ，等到中斷與異常的處理結束後再跳回原本的指令位址繼續執行。
> 補充:
> 當異常或是中斷發生時，處理器會停止手邊的工作，再將 Program counter 的位址指向 mtvec 所指的位址並開始執行。這樣的行為就好像是主動跳入陷阱一樣，因此，在 RISC-V 的架構中將這個動作定義為 Trap ，在 xv6 (risc-v) 作業系統中，我們也可以在 Kernel 端的原始碼找到一系列處理 Interrupt 的操作 (大多定義在 Trap.c 之中)。

### CSR
RISC-V 架構定義了許多暫存器，部分暫存器被定義為控制和狀態暫存器，也就是標題所指出的 CSR (Control and status registers) ，它被用於配置或是紀錄處理器的運作狀況。
 - CSR
     - mtvec
     當進入異常時， PC (Program counter) 會進入 mtvec 所指向的地址並繼續運行。
     - mcause
     紀載異常的原因
     - mtval
     紀載異常訊息
     - mepc
     進入異常前 PC 所指向的地址。若異常處理完畢， Program counter 可以讀取該位址並繼續執行。
     - mstatus
進入異常時，硬體會更新 mstatus 寄存器的某些域值。
     - mie
     決定中斷是否被處理。
     - mip
     反映不同類型中斷的等待狀態。
 - Memory Address Mapped
     - mtime
     紀錄計時器的值。
     - mtimecmp
     儲存計時器的比較值。
     - msip
     產生或結束軟體中斷。
     - PLIC
     
此外， RISC-V 定義了一系列的指令讓開發者能夠對 CSR 暫存器進行操作:
- csrs
把 CSR 中指定的 bit 設為 1。
```assembly=
csrsi mstatus, (1 << 2)
```
上面的指令會將 mstatus 從 LSB 數起的第三個位置設成 1 。
- csrc
把 CSR 中指定的 bit 設為 0。
```assembly=
csrsi mstatus, (1 << 2)
```
上面的指令會將 mstatus 從 LSB 數起的第三個位置設成 0 。
- csrr[c|s]
將 CSR 的值讀入通用暫存器。
```assembly=
csrr to, mscratch
```
- csrw
將通用暫存器的值寫入 CSR 。
```assembly=
csrw	mepc, a0
```
- csrrw[i]
將 csr 的值寫入 rd 後，且將 rs1 的值寫入 csr 。
```assembly=
csrrw rd, csr, rs1/imm
```
換個角度思考:
```assembly=
csrrw t6, mscratch, t6
```
上面的操作可以讓 t6 與 mscratch 的值互換。
## 系統執行

首先讓我們展示一下系統的執行狀況，當你用 make clean, make 等指令建置好
04-TimerInterrupt 下的專案後，就可以用 make qemu 開始執行，結果如下：

```
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
OS start
timer_handler: 1
timer_handler: 2
timer_handler: 3
timer_handler: 4
timer_handler: 5
QEMU: Terminated
```

系統會很穩定的以大約每秒一次的方式，印出 `timer_handler: i` 這樣的訊息，這代表時間中斷機制啟動成功，而且定時進行中斷。

## 主程式 [os.c]

在講解時間中斷之前，先讓我們看看作業系統主程式 [os.c] 的內容。

```cpp
#include "os.h"

int os_main(void)
{
	lib_puts("OS start\n");
	timer_init(); // start timer interrupt ...

	while (1) {} // stop here !
	return 0;
}
```

基本上這個程式印出了 `OS start` 之後，啟動了時間中斷，然後就進入無窮迴圈卡住了。

但是為何該系統之後還會印出 `timer_handler: i` 這樣的訊息呢？

```
timer_handler: 1
timer_handler: 2
timer_handler: 3
```

這當然是因為時間中斷機制造成的！

讓我們看看 [timer.c] 的內容，請特別注意其中的 `w_mtvec((reg_t)sys_timer)` 這行，當時間中斷發生時，程式會跳到 [sys.s] 裏的 sys_timer 這個組合語言函數。

```cpp
#include "timer.h"

extern void os_kernel();

// a scratch area per CPU for machine-mode timer interrupts.
reg_t timer_scratch[NCPU][5];

void timer_init()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid();

  // ask the CLINT for a timer interrupt.
  int interval = 10000000; // cycles; about 1 second in qemu.
  *(reg_t*)CLINT_MTIMECMP(id) = *(reg_t*)CLINT_MTIME + interval;

  // prepare information in scratch[] for timervec.
  // scratch[0..2] : space for timervec to save registers.
  // scratch[3] : address of CLINT MTIMECMP register.
  // scratch[4] : desired interval (in cycles) between timer interrupts.
  reg_t *scratch = &timer_scratch[id][0];
  scratch[3] = CLINT_MTIMECMP(id);
  scratch[4] = interval;
  w_mscratch((reg_t)scratch);

  // set the machine-mode trap handler.
  w_mtvec((reg_t)sys_timer);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable machine-mode timer interrupts.
  w_mie(r_mie() | MIE_MTIE);
}

static int timer_count = 0;

void timer_handler() {
  lib_printf("timer_handler: %d\n", ++timer_count);
  // os_kernel();
}
```

而 [sys.s] 裏的 sys_timer 這個函數，會用 csrr 這個特權指令將 mepc 特權暫存器暫存入 a7 當中，然後設定 mepc = sys_kernel 。

```s
sys_timer:
        # ....
        csrr a7, mepc     # a7 = mepc, for sys_kernel jump back to interrupted point
        la a1, sys_kernel # mepc = sys_kernel
        csrw mepc, a1     # mret : will jump to sys_kernel
        # ....
        mret
```

在這裡讀者必須先理解 RISC-V 的中斷機制，RISC-V 基本上有三種執行模式，那就是《機器模式 machine mode, 超級模式 super mode 與 使用者模式 user mode》。

本書中 mini-riscv-os 的所有範例，都是在機器模式下執行的，沒有使用到 super mode 或 user mode。

而 mepc 就是機器模式下中斷發生時，硬體會自動執行 mepc=pc 的動作。

當 sys_timer 在執行 mret 後，硬體會執行 pc=mepc 的動作，然後就跳回原本的中斷點繼續執行了。(就好像沒發生過甚麼事一樣)

但是我們想要偷樑換柱，讓 sys_timer 在執行到 mret 時，跳到另一個 sys_kernel 函數去執行。

```s
sys_kernel:
        addi sp, sp, -128  # alloc stack space
        reg_save sp        # save all registers
        call timer_handler # call timer_handler in timer.c
        reg_load sp        # restore all registers
        addi sp, sp, 128   # restore stack pointer
        jr a7              # jump to a7=mepc , return to timer break point
```

因此我們在 sys_timer 裏，偷偷的把 mepc 換為 sys_kernel ，並且將原本的 mepc 保存在 a7 暫存器當中。(a7 暫存器預設是函數呼叫時的第七個暫存器，因為參數很少到達 7 個以上，所以很少被用到，我們拿來偷存 mepc，希望不會因此影響到系統行為)

```s
sys_timer:
        # ....
        csrr a7, mepc     # a7 = mepc, for sys_kernel jump back to interrupted point
        la a1, sys_kernel # mepc = sys_kernel
        csrw mepc, a1     # mret : will jump to sys_kernel
        # ....
        mret
```

這樣當 mret 執行時，就會跳到 sys_kernel 去，而不是跳回被打斷那個點。

然後我們在 sys_kernel 裏才去真正呼叫中斷時要做的 C 語言函數 timer_handler。

當然，在呼叫 C 語言函數前必須先把目前暫存器全都透過 reg_save 這個巨集保存起來，呼叫完 timer_handler 之後將暫存器的內容恢復，才能像沒事一樣繼續回到原本的中斷點繼續執行。(否則暫存器被改掉的話，回到中斷點也無法正確地繼續執行了)

```s
sys_kernel:
        addi sp, sp, -128  # alloc stack space
        reg_save sp        # save all registers
        call timer_handler # call timer_handler in timer.c
        reg_load sp        # restore all registers
        addi sp, sp, 128   # restore stack pointer
        jr a7              # jump to a7=mepc , return to timer break point
```

當 timer_handler 呼叫完成，且恢復了暫存器之後，就可以透過 jr a7 這個指令，跳回到當初保存的返回點 (a7=mepc)，於是又回到原本的中斷點繼續執行，好像沒發生過甚麼事一樣了。

以上我們已經將 RISC-V 的中斷機制原理粗略的講解完了，但是更細緻的過程，還得進一步理解 RISC-V 處理器的機器模式 (machine mode) 相關特權暫存器，像是 mhartid (處理器核心代號), mscratch (臨時暫存區起始點), mstatus (狀態暫存器)，mie (中斷暫存器) 等等。

```cpp
#include "timer.h"

extern void os_kernel();

// a scratch area per CPU for machine-mode timer interrupts.
reg_t timer_scratch[NCPU][5];

void timer_init()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid();

  // ask the CLINT for a timer interrupt.
  int interval = 10000000; // cycles; about 1 second in qemu.
  *(reg_t*)CLINT_MTIMECMP(id) = *(reg_t*)CLINT_MTIME + interval;

  // prepare information in scratch[] for timervec.
  // scratch[0..2] : space for timervec to save registers.
  // scratch[3] : address of CLINT MTIMECMP register.
  // scratch[4] : desired interval (in cycles) between timer interrupts.
  reg_t *scratch = &timer_scratch[id][0];
  scratch[3] = CLINT_MTIMECMP(id);
  scratch[4] = interval;
  w_mscratch((reg_t)scratch);

  // set the machine-mode trap handler.
  w_mtvec((reg_t)sys_timer);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable machine-mode timer interrupts.
  w_mie(r_mie() | MIE_MTIE);
}
```

另外還得理解 RISC-V 的 virt 這台 QEMU 虛擬機器中的記憶體映射區域，像是CLINT_MTIME, CLINT_MTIMECMP 等等。

RISC-V 的時間中斷機制是比較 CLINT_MTIME 與 CLINT_MTIMECMP 兩個數值，當 CLINT_MTIME 超過 CLINT_MTIMECMP 時就發生中斷。

因此 timer_init() 函數才會有下列指令

```cpp
  *(reg_t*)CLINT_MTIMECMP(id) = *(reg_t*)CLINT_MTIME + interval;
```

就是為了設定第一次的中斷時間。

同樣的，在 [sys.c] 的 sys_timer 裏，也要去設定下一次的中斷時間:

```s
sys_timer:
        # timer_init() has set up the memory that mscratch points to:
        # scratch[0,4,8] : register save area.
        # scratch[12] : address of CLINT's MTIMECMP register.
        # scratch[16] : desired interval between interrupts.

        csrrw a0, mscratch, a0 #  exchange(mscratch,a0)
        sw a1, 0(a0)
        sw a2, 4(a0)
        sw a3, 8(a0)

        # schedule the next timer interrupt
        # by adding interval to mtimecmp.
        lw a1, 12(a0)  # CLINT_MTIMECMP(hart)
        lw a2, 16(a0)  # interval
        lw a3, 0(a1)   # a3 = CLINT_MTIMECMP(hart)
        add a3, a3, a2 # a3 += interval
        sw a3, 0(a1)   # CLINT_MTIMECMP(hart) = a3

        csrr a7, mepc     # a7 = mepc, for sys_kernel jump back to interrupted point
        la a1, sys_kernel # mepc = sys_kernel
        csrw mepc, a1     # mret : will jump to sys_kernel
 
        lw a3, 8(a0)
        lw a2, 4(a0)
        lw a1, 0(a0)
        csrrw a0, mscratch, a0 # exchange(mscratch,a0)

        mret              # jump to mepc (=sys_kernel)
```

請特別注意其中的這段程式碼，其目的是將 CLINT_MTIMECMP 加上 interval，也就是 CLINT_MTIMECMP += interval 的意思。

```s
        # schedule the next timer interrupt
        # by adding interval to mtimecmp.
        lw a1, 12(a0)  # CLINT_MTIMECMP(hart)
        lw a2, 16(a0)  # interval
        lw a3, 0(a1)   # a3 = CLINT_MTIMECMP(hart)
        add a3, a3, a2 # a3 += interval
        sw a3, 0(a1)   # CLINT_MTIMECMP(hart) = a3
```

這樣下一次 CLINT_MTIMECMP 時間到的時候，CLINT_MTIME 就會大於 CLINT_MTIMECMP，於是中斷就再次發生了。

以上就是 RISC-V 處理器在 virt 這個虛擬機器上的中斷機制原理！



