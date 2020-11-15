# 03-MultiTasking -- RISC-V 的協同式多工

[os.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/03-MultiTasking/os.c

[task.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/03-MultiTasking/task.c

[user.c]:https://github.com/ccc-c/mini-riscv-os/blob/master/03-MultiTasking/user.c

[sys.s]:https://github.com/ccc-c/mini-riscv-os/blob/master/03-MultiTasking/sys.s

專案 -- https://github.com/ccc-c/mini-riscv-os/tree/master/03-MultiTasking

在前一章的 [02-ContextSwitch](02-ContextSwitch.md) 中我們介紹了 RISC-V 架構下的內文切換機制，這一章我們將進入多行程的世界，介紹如何撰寫一個《協同式多工》作業系統。

## 協同式多工

現代的作業系統，都有透過時間中斷強制中止行程的《搶先》(Preemptive) 功能，這樣就能在某行程霸佔 CPU 太久時，強制將其中斷，切換給別的行程執行。

但是在沒有時間中斷機制的系統中，作業系統《無法中斷惡霸行程》，因此必須依賴各個行程主動交回控制權給作業系統，才能讓所有行程都有機會執行。

這種仰賴自動交還機制的多行程系統，稱為《協同式多工》(Coorperative Multitasking) 系統。

1991 年微軟推出的 Windows 3.1 ，還有單板電腦 arduino 上的 [HeliOS](https://github.com/MannyPeterson/HeliOS)，都是《協同式多工》機制的作業系統。

在本章中，我們將設計一個在 RISC-V 處理器上的《協同式多工》作業系統。

首先讓我們來看看該系統的執行結果。

```sh
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
OS start
OS: Activate next task
Task0: Created!
Task0: Now, return to kernel mode
OS: Back to OS

OS: Activate next task
Task1: Created!
Task1: Now, return to kernel mode
OS: Back to OS

OS: Activate next task
Task0: Running...
OS: Back to OS

OS: Activate next task
Task1: Running...
OS: Back to OS

OS: Activate next task
Task0: Running...
OS: Back to OS

OS: Activate next task
Task1: Running...
OS: Back to OS

OS: Activate next task
Task0: Running...
OS: Back to OS

OS: Activate next task
Task1: Running...
OS: Back to OS

OS: Activate next task
Task0: Running...
QEMU: Terminated
```

您可以看到該系統在兩個任務 Task0, Task1 之間不斷切換，但實際上的切換過程如下： 

```
OS=>Task0=>OS=>Task1=>OS=>Task0=>OS=>Task1 ....
```

## 使用者任務 [user.c]

在 [user.c] 中我們定義了 user_task0 與 user_task1 兩個 task，並且在 user_init 函數終將這兩個 task 初始化。

* https://github.com/ccc-c/mini-riscv-os/blob/master/03-MultiTasking/user.c

```cpp
#include "os.h"

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

void user_task1(void)
{
	lib_puts("Task1: Created!\n");
	lib_puts("Task1: Now, return to kernel mode\n");
	os_kernel();
	while (1) {
		lib_puts("Task1: Running...\n");
		lib_delay(1000);
		os_kernel();
	}
}

void user_init() {
	task_create(&user_task0);
	task_create(&user_task1);
}
```

## 主程式 [os.c]

然後在作業系統主程式 os.c 當中，我們使用大輪迴的方式，安排每個行程按照順序輪迴的執行。

* https://github.com/ccc-c/mini-riscv-os/blob/master/03-MultiTasking/os.c

```cpp
#include "os.h"

void os_kernel() {
	task_os();
}

void os_start() {
	lib_puts("OS start\n");
	user_init();
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

上述排程方法原則上和 [Round Robin Scheduling](https://en.wikipedia.org/wiki/Round-robin_scheduling) 一致，但是 Round Robin Scheduling 原則上必須搭配時間中斷機制，但本章的程式碼沒有時間中斷，所以只能說是協同式多工版本的 Round Robin Scheduling。

協同式多工必須依賴各個 task 主動交回控制權，像是在 user_task0 裏，每當呼叫 os_kernel() 函數時，就會呼叫內文切換機制，將控制權交回給作業系統 [os.c] 。

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

其中 [os.c] 的 os_kernel() 會呼叫 [task.c] 的 task_os()

```cpp
void os_kernel() {
	task_os();
}
```

而 task_os() 則會呼叫組合語言 [sys.s] 裏的 sys_switch 去切換回作業系統中。

```cpp
// switch back to os
void task_os() {
	struct context *ctx = ctx_now;
	ctx_now = &ctx_os;
	sys_switch(ctx, &ctx_os);
}
```

於是整個系統就在 os_main(), user_task0(), user_task1() 三者的協作下，以互相禮讓的方式輪流執行著。

[os.c]

```cpp
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

[user.c]

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

void user_task1(void)
{
	lib_puts("Task1: Created!\n");
	lib_puts("Task1: Now, return to kernel mode\n");
	os_kernel();
	while (1) {
		lib_puts("Task1: Running...\n");
		lib_delay(1000);
		os_kernel();
	}
}
```

以上就是 RISC-V 處理器上一個具體而微的協同式多工作業系統範例了！

