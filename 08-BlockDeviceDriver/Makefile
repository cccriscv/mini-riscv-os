CC = riscv64-unknown-elf-gcc
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -march=rv32ima -mabi=ilp32 -g -Wall -w
GDB = riscv64-unknown-elf-gdb

OBJ = \
start.s \
sys.s \
lib.c \
timer.c \
task.c \
os.c \
user.c \
trap.c \
lock.c \
plic.c \
virtio.c \
string.c

QEMU = qemu-system-riscv32
QFLAGS = -nographic -smp 4 -machine virt -bios none
QFLAGS += -drive file=fs.img,if=none,format=raw,id=x0
QFLAGS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

OBJDUMP = riscv64-unknown-elf-objdump

all: os.elf

os.elf: $(OBJ)
	$(CC) $(CFLAGS) -T os.ld -o os.elf $^

qemu: $(TARGET)
	@qemu-system-riscv32 -M ? | grep virt >/dev/null || exit
	@echo "Press Ctrl-A and then X to exit QEMU"
	$(QEMU) $(QFLAGS) -kernel os.elf

clean:
	rm -f *.elf

fs.img:
	qemu-img create -f raw fs.img 50M

.PHONY : debug
debug: all
	@echo "Press Ctrl-C and then input 'quit' to exit GDB and QEMU"
	@echo "-------------------------------------------------------"
	@${QEMU} ${QFLAGS} -kernel os.elf -s -S &
	@${GDB} os.elf -q -x ./gdbinit