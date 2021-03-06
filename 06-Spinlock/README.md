# 06-Spinlock

## Build & Run

```sh
IAN@DESKTOP-9AEMEPL MINGW64 ~/Desktop/mini-riscv-os/06-Spinlock (feat/spinlock)
$ make
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -g -Wall -T os.ld -o os.elf start.s sys.s lib.c timer.c task.c os.c user.c trap.c lock.c

IAN@DESKTOP-9AEMEPL MINGW64 ~/Desktop/mini-riscv-os/06-Spinlock (feat/spinlock)
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
timer interruption!
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
timer interruption!
timer_handler: 2
OS: Back to OS

OS: Activate next task
Task2: Created!
The value of shared_var is: 550
The value of shared_var is: 600
The value of shared_var is: 650
The value of shared_var is: 700
The value of shared_var is: 750
The value of shared_var is: 800
The value of shared_var is: 850
The value of shared_var is: 900
The value of shared_var is: 950
The value of shared_var is: 1000
The value of shared_var is: 1050
The value of shared_var is: 1100
The value of shared_var is: 1150
The value of shared_var is: 1200
The value of shared_var is: 1250
The value of shared_var is: 1300
The value of shared_var is: 1350
The value of shared_var is: 1400
The value of shared_var is: 1450
The value of shared_var is: 1500
The value of shared_var is: 1550
The value of shared_var is: 1600
timer interruption!
timer_handler: 3
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
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
Task0: Running...
timer interruption!
timer_handler: 4
OS: Back to OS
QEMU: Terminated
```

## Debug mode

```sh
make debug
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -g -Wall -T os.ld -o os.elf start.s sys.s lib.c timer.c task.c os.c user.c trap.c lock.c
Press Ctrl-C and then input 'quit' to exit GDB and QEMU
-------------------------------------------------------
Reading symbols from os.elf...
Breakpoint 1 at 0x80000000: file start.s, line 7.
0x00001000 in ?? ()
=> 0x00001000:  97 02 00 00     auipc   t0,0x0

Thread 1 hit Breakpoint 1, _start () at start.s:7
7           csrr t0, mhartid                # read current hart id
=> 0x80000000 <_start+0>:       f3 22 40 f1     csrr    t0,mhartid
(gdb)
```

### set the breakpoint

You can set the breakpoint in any c file:

```sh
(gdb) b trap.c:27
Breakpoint 2 at 0x80008f78: file trap.c, line 27.
(gdb)
```

As the example above, when process running on trap.c, line 27 (Timer Interrupt).
The process will be suspended automatically until you press the key `c` (continue) or `s` (step).
