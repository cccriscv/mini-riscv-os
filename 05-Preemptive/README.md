# 05-Preemptive

## Build & Run

```sh
user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/05-Preemptive (master)       
$ make clean
rm -f *.elf

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/05-Preemptive (master)       
$ make
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s sys.s lib.c timer.c task.c os.c user.c

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/05-Preemptive (master)       
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
Task0: Running...
Task0: Running...
timer_handler: 1
OS: Back to OS

OS: Activate next task
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
timer_handler: 5
OS: Back to OS

OS: Activate next task
Task1: Running...
Task1: Running...
QEMU: Terminated
```
