.equ STACK_SIZE, 8192

.global _start

_start:
    csrr a0, mhartid                # 讀取核心代號
    bnez a0, park                   # 若不是 0 號核心，跳到 park 停止
    la   sp, stacks + STACK_SIZE    # 0 號核心設定堆疊
    j    os_main                    # 0 號核心跳到主程式 os_main

park:
    wfi
    j park

stacks:
    .skip STACK_SIZE                # 分配堆疊空間
