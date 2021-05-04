# 06-Fork -- RISC-V 的嵌入式作業系統

系統呼叫 `fork()` 會複製當前 Process 的內容並創建一份新的 Process 。
關於 `fork()` 的定義與實現，可以參考專案中的 unistd.c 以及 unistd.h :

```c=
/* unistd.c */
#include "unistd.h"
#include "task.h"
#include "os.h"
int fork()
{
  int parent = get_current_task();
  return task_fork(parent);
}
void wait()
{
  task_create(&task_killer);
}
int getpid()
{
  return get_current_task();
}
```

`fork()` 回傳值代表的意義:

- -1 ： 發生錯誤
- 0 ： 代表為子程序
- 大於 0 ： 代表為父程序, 其回傳值為子程序的 ProcessID

此外，為了讓 mini-riscv-os 支援 fork 系統呼叫，我也針對 task.c 以及 os.c 做了一些修改:

```c=
/* os.c */
int current_task = 0;

int get_current_task()
{
	return current_task;
}
```

為了支援 `getpid()` ，我將 current_task 放到全域變數並提供 `get_current_task()` 函式。

```c=
struct context ctx_os;
struct task_node
{
	/* para: parent_id
	 * defaut: -1 (I'm parent)
	 */
	int parent_id;
	struct context ctx;
};
struct task_node ctx_tasks[MAX_TASK];

// ...

int task_fork(int pid)
{
	if (MAX_TASK == taskTop)
		return -1;
	// Parent process
	if (ctx_tasks[pid].parent_id == -1)
	{
		ctx_tasks[taskTop].ctx.ra = ctx_tasks[pid].ctx.ra;
		ctx_tasks[taskTop].ctx.sp = ctx_tasks[pid].ctx.sp;
		ctx_tasks[taskTop].parent_id = pid;
		return taskTop++;
	}
	return 0;
}
```

將 task 額外新增了 parent_id ，如果本身是由 user space 創建， parent_id 會是 `-1` 。
這樣做可以幫助 `task_fork()` 判斷是否要建立 child process ，如果 parent_id 不為 `-1` 就代表這個行程已經是由其他 Process 複製出來的了。

## 殭屍行程 & 孤兒行程

在 UNIX 作業系統中，行程結束後，被分配到的資源都將被回收，除了 task_struct 結構及少數資源外。 此時的程序已經**死亡** ，但 task_struct 結構還儲存在程序列表中， 半死不活 ，故稱為**殭屍行程**。
在回收殭屍程序之前，如果 Parent Process 退出了，則殭屍程序變為 “孤兒行程” ，進而被 init 接管與回收。
為了避免上述情況，我在 task.c 中實作了 `task_killer()` 用來殺死 Child Process 以及 killer 本身。

```c=
void task_killer()
{
	for (int i = 0; i <= 1; i++)
	{
		ctx_tasks[taskTop].ctx.ra = 0;
		ctx_tasks[taskTop].ctx.sp = 0;
		ctx_tasks[taskTop].parent_id = 0;
		taskTop--;
		minus_current_task();
	}
	os_kernel();
}
```

實作 `task_killer()` 後，便可以供 `wait()` 調用，讓作業系統支援簡易的 Child Process 功能。

## 系統執行

```
IAN@DESKTOP-9AEMEPL MINGW64 ~/Desktop/06-Fork
$ make clean
rm -f *.elf

IAN@DESKTOP-9AEMEPL MINGW64 ~/Desktop/06-Fork
$ make all
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s sys.s lib.c timer.c task.c os.c user.c unistd.c

IAN@DESKTOP-9AEMEPL MINGW64 ~/Desktop/06-Fork
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
OS start
OS: Activate next task
Task0: Created!
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
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
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...
timer_handler: 2
OS: Back to OS

OS: Activate next task
Task2: Created!
I'm parent!
OS: Back to OS

OS: Activate next task
Task2: Created!
I'm child!
OS: Back to OS

OS: Activate next task
OS: Back to OS

OS: Activate next task
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
timer_handler: 3
OS: Back to OS

OS: Activate next task
Task1: Running...
Task1: Running...
Task1: Running...
Task1: Running...

```
