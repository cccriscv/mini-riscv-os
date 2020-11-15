# 05-Preemptive -- RISC-V 的嵌入式作業系統

[lib.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/lib.c

[os.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/os.c

[timer.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/timer.c

[sys.s]:https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/sys.s

[task.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/task.c

[user.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/user.c

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

唯一不同的是，第三章的使用者行程必須主動透過 os_kernel() 歸還控制權給作業系統，

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

* https://github.com/ccc-c/mini-riscv-os/blob/master/05-Preemptive/os.c

作業系統 os.c 一開始會呼叫 user_init() ，讓使用者建立 task (在本範例中會在 [user.c] 裏建立 user_task0 與 user_task1。

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

然後作業系統會在 os_start() 裏透過 timer_init() 函數設定時間中斷，接著就是進入 os_main() 的主迴圈裏，該迴圈採用 Round-Robin 的大輪迴排班方法，每次切換就選下一個 task 來執行 (若已到最後一個 task ，接下來就是第 0 個 task)。

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

05-Preemptive 的時間中斷原理和上一章相同，都是在 [timer.c] 的 timer_init() 中設定好第一次的時間中斷。

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
  // int interval = 1000000; // cycles; about 1/10th second in qemu.
  int interval = 20000000; // cycles; about 2 second in qemu.
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
  os_kernel();
}

```

但不同的是，[sys.s] 裏的 sys_timer 會呼叫上面 [timer.c] 裏的 timer_handler()，其中包含了 `os_kernel()` 這個函數，該函數會呼叫 [task.c] 裏的 task_os()， task_os() 會呼叫 [sys.s] 裏的 sys_switch 去切換回 kernel，於是作業系統就透過時間中斷將控制權強制取回來了。

```s
# ...
sys_switch:
        ctx_save a0  # a0 => struct context *old
        ctx_load a1  # a1 => struct context *new
        ret          # pc=ra; swtch to new task (new->ra)
# ...
sys_kernel:
        addi sp, sp, -128  # alloc stack space
        reg_save sp        # save all registers
        call timer_handler # call timer_handler in timer.c
        reg_load sp        # restore all registers
        addi sp, sp, 128   # restore stack pointer
        jr a7              # jump to a7=mepc , return to timer break point
# ...
sys_timer:
# ...
        csrr a7, mepc     # a7 = mepc, for sys_kernel jump back to interrupted point
        la a1, sys_kernel # mepc = sys_kernel
        csrw mepc, a1     # mret : will jump to sys_kernel
# ...
```

透過時間中斷強制取回控制權，我們就不用擔心有惡霸行程把持 CPU 不放，系統也就不會被惡霸卡住而整個癱瘓了，這就是現代作業系統中最重要的《行程管理機制》。

雖然 mini-riscv-os 只是個微型的嵌入式作業系統，但是仍然透過相對精簡的程式碼，示範了一個具體而微的《可搶先作業系統》之設計原理。

當然，學習《作業系統設計》的道路還很長，mini-riscv-os 沒有《檔案系統》，而且我還沒學會 RISC-V 當中的 super mode 與 user mode 之控制與切換方式，也還沒引入 RISC-V 的虛擬記憶體機制，因此本章的程式碼仍然只有使用 machine mode，因此沒辦法提供較完整的《權限與保護機制》。

還好，這些事情已經有人做好了，您可以透過學習 xv6-riscv 這個由 MIT 所設計的教學型作業系統，進一步了解這些較複雜的機制，xv6-riscv 的原始碼總共有八千多行，雖然不算太少，但是比起那些動則數百萬行到數千萬行的 Linux / Windows 而言，xv6-riscv 算是非常精簡的系統了。

* https://github.com/mit-pdos/xv6-riscv

然而 xv6-riscv 原本只能在 linux 下編譯執行，但是我把其中的 mkfs/mkfs.c 修改了一下，就能在 windows + git bash 這樣和 mini-riscv-os 一樣的環境下編譯執行了。

您可以從下列網址中取得 windows 版的 xv6-riscv 原始碼，然後編譯執行看看，應該可以站在 mini-riscv-os 的基礎上，進一步透過 xv6-riscv 學習更進階的作業系統設計原理。

* https://github.com/ccc-c/xv6-riscv-win

以下提供更多關於 RISC-V 的學習資源，以方便大家在學習 RISC-V 作業系統設計時，不需再經過太多的摸索。

* [RISC-V 手册 - 一本开源指令集的指南 (PDF)](http://crva.ict.ac.cn/documents/RISC-V-Reader-Chinese-v2p1.pdf)
* [The RISC-V Instruction Set Manual Volume II: Privileged Architecture Privileged Architecture (PDF)](https://riscv.org//wp-content/uploads/2019/12/riscv-spec-20191213.pdf)
* [RISC-V Assembly Programmer's Manual](https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md)
* https://github.com/riscv/riscv-opcodes
    * https://github.com/riscv/riscv-opcodes/blob/master/opcodes-rv32i
* [SiFive Interrupt Cookbook (SiFive 的 RISC-V 中斷手冊)](https://gitlab.com/ccc109/sp/-/blob/master/10-riscv/mybook/riscv-interrupt/sifive-interrupt-cookbook-zh.md)
* [SiFive Interrupt Cookbook -- Version 1.0 (PDF)](https://sifive.cdn.prismic.io/sifive/0d163928-2128-42be-a75a-464df65e04e0_sifive-interrupt-cookbook.pdf)
* 進階: [proposal for a RISC-V Core-Local Interrupt Controller (CLIC)](https://github.com/riscv/riscv-fast-interrupt/blob/master/clic.adoc)

希望這份 mini-riscv-os 教材能幫助讀者在學習 RISC-V OS 設計上節省一些寶貴的時間！

               陳鍾誠 2020/11/15 於金門大學




