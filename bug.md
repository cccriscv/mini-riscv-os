所有 start.s 中的 .equ STACK_SIZE, 8192 應該都要改為 .equ STACK_SIZE, 1024

因為 後面的  slli t0, t0, 10 # shift left the hart id by 1024

這兩個要一致，所以不是把 STACK_SIZE 改為 8192, 就是把  slli t0, t0, 10 的 10 改為 13



