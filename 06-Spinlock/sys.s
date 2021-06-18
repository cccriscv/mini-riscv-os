# This Code derived from xv6-riscv (64bit)
# -- https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/swtch.S

# ============ MACRO ==================
.macro ctx_save base
        sw ra, 0(\base)
        sw sp, 4(\base)
        sw s0, 8(\base)
        sw s1, 12(\base)
        sw s2, 16(\base)
        sw s3, 20(\base)
        sw s4, 24(\base)
        sw s5, 28(\base)
        sw s6, 32(\base)
        sw s7, 36(\base)
        sw s8, 40(\base)
        sw s9, 44(\base)
        sw s10, 48(\base)
        sw s11, 52(\base)
.endm

.macro ctx_load base
        lw ra, 0(\base)
        lw sp, 4(\base)
        lw s0, 8(\base)
        lw s1, 12(\base)
        lw s2, 16(\base)
        lw s3, 20(\base)
        lw s4, 24(\base)
        lw s5, 28(\base)
        lw s6, 32(\base)
        lw s7, 36(\base)
        lw s8, 40(\base)
        lw s9, 44(\base)
        lw s10, 48(\base)
        lw s11, 52(\base)
.endm

.macro reg_save base
        # save the registers.
        sw ra, 0(\base)
        sw sp, 4(\base)
        sw gp, 8(\base)
        sw tp, 12(\base)
        sw t0, 16(\base)
        sw t1, 20(\base)
        sw t2, 24(\base)
        sw s0, 28(\base)
        sw s1, 32(\base)
        sw a0, 36(\base)
        sw a1, 40(\base)
        sw a2, 44(\base)
        sw a3, 48(\base)
        sw a4, 52(\base)
        sw a5, 56(\base)
        sw a6, 60(\base)
        sw a7, 64(\base)
        sw s2, 68(\base)
        sw s3, 72(\base)
        sw s4, 76(\base)
        sw s5, 80(\base)
        sw s6, 84(\base)
        sw s7, 88(\base)
        sw s8, 92(\base)
        sw s9, 96(\base)
        sw s10, 100(\base)
        sw s11, 104(\base)
        sw t3, 108(\base)
        sw t4, 112(\base)
        sw t5, 116(\base)
        sw t6, 120(\base)
.endm

.macro reg_load base
        # restore registers.
        lw ra, 0(\base)
        lw sp, 4(\base)
        lw gp, 8(\base)
        # not this, in case we moved CPUs: lw tp, 12(\base)
        lw t0, 16(\base)
        lw t1, 20(\base)
        lw t2, 24(\base)
        lw s0, 28(\base)
        lw s1, 32(\base)
        lw a0, 36(\base)
        lw a1, 40(\base)
        lw a2, 44(\base)
        lw a3, 48(\base)
        lw a4, 52(\base)
        lw a5, 56(\base)
        lw a6, 60(\base)
        lw a7, 64(\base)
        lw s2, 68(\base)
        lw s3, 72(\base)
        lw s4, 76(\base)
        lw s5, 80(\base)
        lw s6, 84(\base)
        lw s7, 88(\base)
        lw s8, 92(\base)
        lw s9, 96(\base)
        lw s10, 100(\base)
        lw s11, 104(\base)
        lw t3, 108(\base)
        lw t4, 112(\base)
        lw t5, 116(\base)
        lw t6, 120(\base)
.endm
# ============ Macro END   ==================
 
# Context switch
#
#   void sys_switch(struct context *old, struct context *new);
# 
# Save current registers in old. Load from new.

.globl sys_switch
.align 4
sys_switch:

        ctx_save a0  # a0 => struct context *old
        ctx_load a1  # a1 => struct context *new
        ret          # pc=ra; swtch to new task (new->ra)

.globl atomic_swap
.align 4
atomic_swap:
        li a5, 1
        amoswap.w.aq a5, a5, 0(a0)
        mv a0, a5
        ret

.globl trap_vector
# the trap vector base address must always be aligned on a 4-byte boundary
.align 4
trap_vector:
	# save context(registers).
	csrrw	t6, mscratch, t6	# swap t6 and mscratch
        reg_save t6
	csrw	mscratch, t6
	# call the C trap handler in trap.c
	csrr	a0, mepc
	csrr	a1, mcause
	call	trap_handler

	# trap_handler will return the return address via a0.
	csrw	mepc, a0

	# load context(registers).
	csrr	t6, mscratch
	reg_load t6
	mret

