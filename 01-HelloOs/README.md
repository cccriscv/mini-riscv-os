# 01-Hello OS

## Build & Run

```sh
user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/01-HelloOs (master)
$ make clean
rm -f *.elf

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/01-HelloOs (master)
$ make 
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s os.c

user@DESKTOP-96FRN6B MINGW64 /d/ccc109/sp/11-os/mini-riscv-os/01-HelloOs (master)
$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
Hello OS!
```
