CC = riscv64-unknown-elf-gcc
CFLAGS = -I./include -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -g -Wall -w -D CONFIG_SYSCALL
GDB = riscv64-unknown-elf-gdb
SOURCE = src/

OBJ = \
$(SOURCE)start.s \
$(SOURCE)sys.s \
$(SOURCE)mem.s \
$(SOURCE)lib.c \
$(SOURCE)timer.c \
$(SOURCE)os.c \
$(SOURCE)task.c \
$(SOURCE)user.c \
$(SOURCE)trap.c \
$(SOURCE)lock.c \
$(SOURCE)plic.c \
$(SOURCE)virtio.c \
$(SOURCE)string.c \
$(SOURCE)alloc.c \
$(SOURCE)syscall.c \
$(SOURCE)usys.s \

QEMU = qemu-system-riscv32
QFLAGS = -nographic -smp 4 -machine virt -bios none
QFLAGS += -drive if=none,format=raw,file=hdd.dsk,id=x0
QFLAGS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

OBJDUMP = riscv64-unknown-elf-objdump

all: clean os.elf hdd.dsk qemu

test: clean os.elf qemu

os.elf: $(OBJ)
	$(CC) $(CFLAGS) -T os.ld -o os.elf $^

qemu: $(TARGET) hdd.dsk
	@qemu-system-riscv32 -M ? | grep virt >/dev/null || exit
	@echo "Press Ctrl-A and then X to exit QEMU"
	$(QEMU) $(QFLAGS) -kernel os.elf

clean:
	rm -f *.elf *.img

hdd.dsk:
	dd if=/dev/urandom of=hdd.dsk bs=1048576 count=32

.PHONY : debug
debug: clean os.elf hdd.dsk
	@echo "Press Ctrl-C and then input 'quit' to exit GDB and QEMU"
	@echo "-------------------------------------------------------"
	@${QEMU} ${QFLAGS} -kernel os.elf -s -S &
	@${GDB} os.elf -q -x ./gdbinit
