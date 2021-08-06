# 05-Preemptive -- RISC-V 的嵌入式作業系統

[lib.c]: https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/lib.c
[os.c]: https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/os.c
[timer.c]: https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/timer.c
[sys.s]: https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/sys.s
[task.c]: https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/task.c
[user.c]: https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/user.c

專案 -- https://github.com/ccc-c/mini-riscv-os/tree/master/05-Preemptive

第三章的 [03-MultiTasking](03-MultiTasking.md) 中我們實作了一個《協同式多工》作業系統。不過由於沒有引入時間中斷機制，無法成為一個《搶先式》(Preemptive) 多工系統。

第四章的 [04-TimerInterrupt](04-TimerInterrupt.md) 中我們示範了 RISC-V 的時間中斷機制原理。

終於到了第五章，我們打算結合前兩章的技術，實作一個具有強制時間中斷的《可搶先式》(Preemptive) 作業系統。這樣的系統就可以算是一個微型的嵌入式作業系統了。

## 系統執行

首先讓我們和看系統的執行狀況，您可以看到下列執行結果中，系統在 OS, Task0, Task1 之間輪流的切換著。

```sh
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
OS start
OS: Activate next task
Task0: Created!
Task0: Running...
Task0: Running...
Task0: Running...
timer_handler: 1
OS: Back to OS

OS: Activate next task
Task1: Created!
Task1: Running...
Task1: Running...
Task1: Running...
timer_handler: 2
OS: Back to OS

OS: Activate next task
Task0: Running...
Task0: Running...
Task0: Running...
timer_handler: 3
OS: Back to OS

OS: Activate next task
Task1: Running...
Task1: Running...
Task1: Running...
timer_handler: 4
OS: Back to OS

OS: Activate next task
Task0: Running...
Task0: Running...
Task0: Running...
QEMU: Terminated
```

這個狀況和第三章的 [03-MultiTasking](03-MultiTasking.md) 非常類似，都是如下的執行順序。

```
OS=>Task0=>OS=>Task1=>OS=>Task0=>OS=>Task1 ....
```

唯一不同的是，第三章的使用者行程必須主動透過 `os_kernel()` 歸還控制權給作業系統，

```cpp
void user_task0(void)
{
	lib_puts("Task0: Created!\n");
	lib_puts("Task0: Now, return to kernel mode\n");
	os_kernel();
	while (1) {
		lib_puts("Task0: Running...\n");
		lib_delay(1000);
		os_kernel();
	}
}
```

但是在本章的 [05-Preemptive](05-Preemptive.md) 中，使用者行程不需要主動交還給 OS，而是由 OS 透過時間中斷強制進行切換動作。

```cpp
void user_task0(void)
{
	lib_puts("Task0: Created!\n");
	while (1) {
		lib_puts("Task0: Running...\n");
		lib_delay(1000);
	}
}
```

其中的 [lib.c] 裏的 lib_delay 其實是個延遲迴圈，並不會交還控制權。

```cpp
void lib_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}
```

相反的，作業系統會透過時間中斷，強制取回控制權。(由於 lib_delay 延遲較久，所以作業系統通常會打斷其 `while (count--)` 的迴圈取回控制權)

## 作業系統 [os.c]

- https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/os.c

作業系統 os.c 一開始會呼叫 `user_init()` ，讓使用者建立 task (在本範例中會在 [user.c] 裏建立 user_task0 與 user_task1。

```cpp
#include "os.h"

void user_task0(void)
{
	lib_puts("Task0: Created!\n");
	while (1) {
		lib_puts("Task0: Running...\n");
		lib_delay(1000);
	}
}

void user_task1(void)
{
	lib_puts("Task1: Created!\n");
	while (1) {
		lib_puts("Task1: Running...\n");
		lib_delay(1000);
	}
}

void user_init() {
	task_create(&user_task0);
	task_create(&user_task1);
}
```

然後作業系統會在 `os_start()` 裏透過 `timer_init()` 函數設定時間中斷，接著就是進入 `os_main()` 的主迴圈裏，該迴圈採用 Round-Robin 的大輪迴排班方法，每次切換就選下一個 task 來執行 (若已到最後一個 task ，接下來就是第 0 個 task)。

```cpp

#include "os.h"

void os_kernel() {
	task_os();
}

void os_start() {
	lib_puts("OS start\n");
	user_init();
	timer_init(); // start timer interrupt ...
}

int os_main(void)
{
	os_start();

	int current_task = 0;
	while (1) {
		lib_puts("OS: Activate next task\n");
		task_go(current_task);
		lib_puts("OS: Back to OS\n");
		current_task = (current_task + 1) % taskTop; // Round Robin Scheduling
		lib_puts("\n");
	}
	return 0;
}
```

在 05-Preemptive 的中斷機制中，我們修改了中斷向量表:

```cpp
.globl trap_vector
# the trap vector base address must always be aligned on a 4-byte boundary
.align 4
trap_vector:
	# save context(registers).
	csrrw	t6, mscratch, t6	# swap t6 and mscratch
        reg_save t6
	csrw	mscratch, t6
	# call the C trap handler in trap.c
	csrr	a0, mepc
	csrr	a1, mcause
	call	trap_handler

	# trap_handler will return the return address via a0.
	csrw	mepc, a0

	# load context(registers).
	csrr	t6, mscratch
	reg_load t6
	mret
```

當中斷發生時，中斷向量表 `trap_vector()` 會呼叫 `trap_handler()` :

```cpp
reg_t trap_handler(reg_t epc, reg_t cause)
{
  reg_t return_pc = epc;
  reg_t cause_code = cause & 0xfff;

  if (cause & 0x80000000)
  {
    /* Asynchronous trap - interrupt */
    switch (cause_code)
    {
    case 3:
      lib_puts("software interruption!\n");
      break;
    case 7:
      lib_puts("timer interruption!\n");
      // disable machine-mode timer interrupts.
      w_mie(~((~r_mie()) | (1 << 7)));
      timer_handler();
      return_pc = (reg_t)&os_kernel;
      // enable machine-mode timer interrupts.
      w_mie(r_mie() | MIE_MTIE);
      break;
    case 11:
      lib_puts("external interruption!\n");
      break;
    default:
      lib_puts("unknown async exception!\n");
      break;
    }
  }
  else
  {
    /* Synchronous trap - exception */
    lib_puts("Sync exceptions!\n");
    while (1)
    {
      /* code */
    }
  }
  return return_pc;
}
```

跳到 `trap_handler()` 之後，它會針對不同類型的中斷呼叫不同的 handler ，所以我們可以將它視為一個中斷的派發任務中繼站:

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

`trap_handler` 可以根據不同的中斷類型，將中斷處理交給不同的 handler ，這樣子做就可以大大的提高作業系統的擴充性。

```cpp
#include "timer.h"

// a scratch area per CPU for machine-mode timer interrupts.
reg_t timer_scratch[NCPU][5];

#define interval 20000000 // cycles; about 2 second in qemu.

void timer_init()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid();

  // ask the CLINT for a timer interrupt.
  // int interval = 1000000; // cycles; about 1/10th second in qemu.

  *(reg_t *)CLINT_MTIMECMP(id) = *(reg_t *)CLINT_MTIME + interval;

  // prepare information in scratch[] for timervec.
  // scratch[0..2] : space for timervec to save registers.
  // scratch[3] : address of CLINT MTIMECMP register.
  // scratch[4] : desired interval (in cycles) between timer interrupts.
  reg_t *scratch = &timer_scratch[id][0];
  scratch[3] = CLINT_MTIMECMP(id);
  scratch[4] = interval;
  w_mscratch((reg_t)scratch);

  // enable machine-mode timer interrupts.
  w_mie(r_mie() | MIE_MTIE);
}

static int timer_count = 0;

void timer_handler()
{
  lib_printf("timer_handler: %d\n", ++timer_count);
  int id = r_mhartid();
  *(reg_t *)CLINT_MTIMECMP(id) = *(reg_t *)CLINT_MTIME + interval;
}

```

看到 [timer.c] 裏的 `timer_handler()`，它會將 `MTIMECMP` 做 reset 的動作。

```cpp
/* In trap_handler() */
// ...
case 7:
      lib_puts("timer interruption!\n");
      // disable machine-mode timer interrupts.
      w_mie(~((~r_mie()) | (1 << 7)));
      timer_handler();
      return_pc = (reg_t)&os_kernel;
      // enable machine-mode timer interrupts.
      w_mie(r_mie() | MIE_MTIE);
      break;
// ...
```

- 為了避免 Timer Interrupt 出現中断嵌套的情況，在處理中斷之前， `trap_handler()` 會將 timer interrupt 關閉，等到處理完成後再打開。
- `timer_handler()` 執行完畢後， `trap_handler()` 會將 mepc 指向 `os_kernel()` ，做到任務切換的功能。
  換言之，如果中斷不屬於 Timer Interrupt ， Program counter 則會跳回進入中斷前的狀態，這個步驟定義在 `trap_vector()` 中:

```assembly=
csrr	a0, mepc # a0 => arg1 (return_pc) of trap_handler()
```

> **補充**
> 在 RISC-V 中，函式的參數會被優先存進 a0 - a7 暫存器，如果不夠用，才會存入 Stack 。
> 其中， a0 與 a1 暫存器還有作為函式返回值的作用。

最後，記得在 Kernel 開機時導入 trap 以及 timer 的初始化動作:

```cpp
void os_start()
{
	lib_puts("OS start\n");
	user_init();
	trap_init();
	timer_init(); // start timer interrupt ...
}
```

透過時間中斷強制取回控制權，我們就不用擔心有惡霸行程把持 CPU 不放，系統也就不會被惡霸卡住而整個癱瘓了，這就是現代作業系統中最重要的《行程管理機制》。

雖然 mini-riscv-os 只是個微型的嵌入式作業系統，但是仍然透過相對精簡的程式碼，示範了一個具體而微的《可搶先作業系統》之設計原理。

當然，學習《作業系統設計》的道路還很長，mini-riscv-os 沒有《檔案系統》，而且我還沒學會 RISC-V 當中的 super mode 與 user mode 之控制與切換方式，也還沒引入 RISC-V 的虛擬記憶體機制，因此本章的程式碼仍然只有使用 machine mode，因此沒辦法提供較完整的《權限與保護機制》。

還好，這些事情已經有人做好了，您可以透過學習 xv6-riscv 這個由 MIT 所設計的教學型作業系統，進一步了解這些較複雜的機制，xv6-riscv 的原始碼總共有八千多行，雖然不算太少，但是比起那些動則數百萬行到數千萬行的 Linux / Windows 而言，xv6-riscv 算是非常精簡的系統了。

- https://github.com/mit-pdos/xv6-riscv

然而 xv6-riscv 原本只能在 linux 下編譯執行，但是我把其中的 mkfs/mkfs.c 修改了一下，就能在 windows + git bash 這樣和 mini-riscv-os 一樣的環境下編譯執行了。

您可以從下列網址中取得 windows 版的 xv6-riscv 原始碼，然後編譯執行看看，應該可以站在 mini-riscv-os 的基礎上，進一步透過 xv6-riscv 學習更進階的作業系統設計原理。

- https://github.com/ccc-c/xv6-riscv-win

以下提供更多關於 RISC-V 的學習資源，以方便大家在學習 RISC-V 作業系統設計時，不需再經過太多的摸索。

- [AwesomeCS Wiki](https://github.com/ianchen0119/AwesomeCS/wiki)
- [Step by step, learn to develop an operating system on RISC-V](https://github.com/plctlab/riscv-operating-system-mooc)
- [RISC-V 手册 - 一本开源指令集的指南 (PDF)](http://crva.ict.ac.cn/documents/RISC-V-Reader-Chinese-v2p1.pdf)
- [The RISC-V Instruction Set Manual Volume II: Privileged Architecture Privileged Architecture (PDF)](https://riscv.org//wp-content/uploads/2019/12/riscv-spec-20191213.pdf)
- [RISC-V Assembly Programmer's Manual](https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md)
- https://github.com/riscv/riscv-opcodes
  - https://github.com/riscv/riscv-opcodes/blob/master/opcodes-rv32i
- [SiFive Interrupt Cookbook (SiFive 的 RISC-V 中斷手冊)](https://gitlab.com/ccc109/sp/-/blob/master/10-riscv/mybook/riscv-interrupt/sifive-interrupt-cookbook-zh.md)
- [SiFive Interrupt Cookbook -- Version 1.0 (PDF)](https://sifive.cdn.prismic.io/sifive/0d163928-2128-42be-a75a-464df65e04e0_sifive-interrupt-cookbook.pdf)
- 進階: [proposal for a RISC-V Core-Local Interrupt Controller (CLIC)](https://github.com/riscv/riscv-fast-interrupt/blob/master/clic.adoc)

希望這份 mini-riscv-os 教材能幫助讀者在學習 RISC-V OS 設計上節省一些寶貴的時間！

               陳鍾誠 2020/11/15 於金門大學
