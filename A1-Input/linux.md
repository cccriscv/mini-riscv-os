

```
guest@localhost:~/sp/10-riscv/03-mini-riscv-os/01-HelloOs$ ls
01-HelloOs--嵌入式RISC-V輸出入程式.md  Makefile  os.c  os.ld  run.md  start.s
guest@localhost:~/sp/10-riscv/03-mini-riscv-os/01-HelloOs$ make 
riscv64-unknown-elf-gcc -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -T os.ld -o os.elf start.s os.c
guest@localhost:~/sp/10-riscv/03-mini-riscv-os/01-HelloOs$ make qemu
Press Ctrl-A and then X to exit QEMU
qemu-system-riscv32 -nographic -smp 4 -machine virt -bios none -kernel os.elf
Hello OS!
QEMU: Terminated
```