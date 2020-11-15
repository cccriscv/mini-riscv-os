# 04-TimerInterrupt

[os.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/04-TimerInterrupt/os.c

[timer.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/04-TimerInterrupt/timer.c

[sys.s]:https://github.com/ccc-c/mini-riscv-os/blob/master/04-TimerInterrupt/sys.s

專案 -- https://github.com/ccc-c/mini-riscv-os/tree/master/04-TimerInterrupt

在前一章的 [03-MultiTasking](03-MultiTasking.md) 中我們實作了一個《協同式多工》作業系統。但是由於沒有引入時間中斷機制，無法成為一個《搶先式》(Preemptive) 多工系統。

本章將為第五章的《搶先式多工系統》鋪路，介紹如何在 RISC-V 處理器中使用《時間中斷機制》。有了時間中斷之後，我們就可以定時強制取回控制權，而不用害怕惡霸行程佔據系統而不歸還控制權給作業系統了。

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



