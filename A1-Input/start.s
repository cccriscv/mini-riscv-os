# 來源 -- https://matrix89.github.io/writes/writes/experiments-in-riscv/
.equ STACK_SIZE, 8192

.global _start

_start:
    # setup stacks per hart // hart: hardware thread, 差不多就是 core, 設定此 core 的堆疊
    csrr t0, mhartid                # read current hart id // 讀取 core 的 id
    slli t0, t0, 10                 # shift left the hart id by 1024 // t0 = t0*1024
    la   sp, stacks + STACK_SIZE    # set the initial stack pointer // 將堆疊指標 sp 放到堆疊底端
                                    # to the end of the stack space
    add  sp, sp, t0                 # move the current hart stack pointer // 設定 hart 的堆疊位置
                                    # to its place in the stack space

    # park harts with id != 0
    csrr a0, mhartid                # read current hart id
    bnez a0, park                   # if we're not on the hart 0 // 如果不是 hart 0 ，就進入無窮迴圈。
                                    # we park the hart

    j    os_main                    # hart 0 jump to c // 堆疊設好了，讓 hart 0 進入 C 語言主程式。

park:                               # 無窮迴圈，停住!
    wfi
    j park

stacks:
    .skip STACK_SIZE * 4            # allocate space for the harts stacks // 分配堆疊空間
