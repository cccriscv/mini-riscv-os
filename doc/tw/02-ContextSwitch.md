# 02-ContextSwitch -- RISC-V 的內文切換

專案 -- https://github.com/ccc-c/mini-riscv-os/tree/master/02-ContextSwitch

在前一章的 [01-HelloOs](01-HelloOs.md) 中我們介紹了如何在 RISC-V 架構下印出字串到 UART 序列埠的方法，這一章我們將往作業系統邁進，介紹神秘的《內文切換》(Context-Switch) 技術。

## os.c

以下是 02-ContextSwitch 的主程式 os.c ，該程式除了 os 本身以外，還有一個《任務》(task)。

* https://github.com/ccc-c/mini-riscv-os/blob/master/02-ContextSwitch/os.c

```cpp
#include "os.h"

#define STACK_SIZE 1024
uint8_t task0_stack[STACK_SIZE];
struct context ctx_os;
struct context ctx_task;

extern void sys_switch();

void user_task0(void)
{
	lib_puts("Task0: Context Switch Success !\n");
	while (1) {} // stop here.
}

int os_main(void)
{
	lib_puts("OS start\n");
	ctx_task.ra = (reg_t) user_task0;
	ctx_task.sp = (reg_t) &task0_stack[STACK_SIZE-1];
	sys_switch(&ctx_os, &ctx_task);
	return 0;
}
```

任務 task 是一個函數，在上面的 os.c 裡是 user_task0，為了進行切換，我們將 ctx_task.ra 設為 user_task0，由於 ra 是 return address 暫存器，其功能為在函數返回執行 ret 指令時，用 ra 取代程式計數器 pc，這樣在執行 ret 指令時就能跳到該函數去執行。

```cpp
	ctx_task.ra = (reg_t) user_task0;
	ctx_task.sp = (reg_t) &task0_stack[STACK_SIZE-1];
	sys_switch(&ctx_os, &ctx_task);
```

但是每個任務都必須有堆疊空間，才能在 C 語言環境中進行函數呼叫。所以我們分配了 task0 的堆疊空間，並用 ctx_task.sp 指向堆疊開頭。

然後我們呼叫了 `sys_switch(&ctx_os, &ctx_task)` 從主程式切換到 task0，其中的 sys_switch 是個位於 [sys.s](https://github.com/ccc-c/mini-riscv-os/blob/master/02-ContextSwitch/sys.s) 裏組合語言函數，內容如下：

```s
# Context switch
#
#   void sys_switch(struct context *old, struct context *new);
# 
# Save current registers in old. Load from new.

.globl sys_switch
.align 4
sys_switch:
        ctx_save a0  # a0 => struct context *old
        ctx_load a1  # a1 => struct context *new
        ret          # pc=ra; swtch to new task (new->ra)
```

在 RISC-V 中，參數主要放在 a0, a1, ..., a7 這些暫存器當中，當參數超過八個時，才會放在堆疊裏傳遞。

sys_switch 對應的 C 語言函數如下：

```cpp
void sys_switch(struct context *old, struct context *new);
```

上述程式的 a0 對應 old (舊任務的 context)，a1 對應 new (新任務的context)，整個 sys_switch 的功能是儲存舊任務的 context ，然後載入新任務 context 開始執行。

最後的一個 ret 指令非常重要，因為當新任務的 context 載入時會把 ra 暫存器也載進來，於是當 ret 執行時，就會設定 pc=ra，然後跳到新任務 (例如 `void user_task0(void)` 去執行了。

sys_switch 中的 `ctx_save` 和 `ctx_load` 是兩個組合語言巨集，其定義如下：

```s
# ============ MACRO ==================
.macro ctx_save base
        sw ra, 0(\base)
        sw sp, 4(\base)
        sw s0, 8(\base)
        sw s1, 12(\base)
        sw s2, 16(\base)
        sw s3, 20(\base)
        sw s4, 24(\base)
        sw s5, 28(\base)
        sw s6, 32(\base)
        sw s7, 36(\base)
        sw s8, 40(\base)
        sw s9, 44(\base)
        sw s10, 48(\base)
        sw s11, 52(\base)
.endm

.macro ctx_load base
        lw ra, 0(\base)
        lw sp, 4(\base)
        lw s0, 8(\base)
        lw s1, 12(\base)
        lw s2, 16(\base)
        lw s3, 20(\base)
        lw s4, 24(\base)
        lw s5, 28(\base)
        lw s6, 32(\base)
        lw s7, 36(\base)
        lw s8, 40(\base)
        lw s9, 44(\base)
        lw s10, 48(\base)
        lw s11, 52(\base)
.endm
# ============ Macro END   ==================
```

RISC-V 行程切換時必須儲存 ra, sp, s0, ... s11 等暫存器，上述的程式碼基本上是我從 xv6 這個教學作業系統中抄來後修改為 RISC-V 32 位元版的，其原始網址如下：

* https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/swtch.S

在 [riscv.h](https://github.com/ccc-c/mini-riscv-os/blob/master/02-ContextSwitch/riscv.h) 這個表頭檔中，我們定義了 struct context 這個對應的 C 語言結構，其內容如下：

```cpp
// Saved registers for kernel context switches.
struct context {
  reg_t ra;
  reg_t sp;

  // callee-saved
  reg_t s0;
  reg_t s1;
  reg_t s2;
  reg_t s3;
  reg_t s4;
  reg_t s5;
  reg_t s6;
  reg_t s7;
  reg_t s8;
  reg_t s9;
  reg_t s10;
  reg_t s11;
};
```

這樣，我們就介紹完《內文切換》任務的細節了，於是下列主程式就能順利地從 os_main 切換到 user_task0 了。

```cpp
int os_main(void)
{
	lib_puts("OS start\n");
	ctx_task.ra = (reg_t) user_task0;
	ctx_task.sp = (reg_t) &task0_stack[STACK_SIZE-1];
	sys_switch(&ctx_os, &ctx_task);
	return 0;
}
```

以下是整個專案的執行結果：

```sh
user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/03-ContextSwitch (master)    
$ make 
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s sys.s lib.c os.c

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/03-ContextSwitch (master)    
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
OS start
Task0: Context Switch Success !
QEMU: Terminated
```

以上就是 RISC-V 裏的《內文切換》(Context-Switch) 機制的實作方法！
