# 09-MemoryAllocator

## Build & Run

```sh
IAN@DESKTOP-9AEMEPL MINGW64 ~/Desktop/mini-riscv-os/09-MemoryAllocator (feat/memoryAlloc)
$ make all
rm -f *.elf *.img
riscv64-unknown-elf-gcc -I./include -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -g -Wall -w -T os.ld -o os.elf src/start.s src/sys.s src/mem.s src/lib.c src/timer.c src/task.c src/os.c src/user.c src/trap.c src/lock.c src/plic.c src/virtio.c src/string.c src/alloc.c
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -drive if=none,format=raw,file=hdd.dsk,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -kernel os.elf
HEAP_START = 8001100c, HEAP_SIZE = 07feeff4, num of pages = 521967
TEXT:   0x80000000 -> 0x8000ac78
RODATA: 0x8000ac78 -> 0x8000b09f
DATA:   0x8000c000 -> 0x8000c004
BSS:    0x8000d000 -> 0x8001100c
HEAP:   0x80091100 -> 0x88000000
OS start
Disk init work is success!
buffer init...
block read...
Virtio IRQ
000000fd
000000af
000000f8
000000ab
00000088
00000042
000000cc
00000017
00000022
0000008e

p = 0x80091700
p2 = 0x80091300
p3 = 0x80091100
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
