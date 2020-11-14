


像是 07-thread 裏使用了 tcb_t 

```cpp
/* Thread Control Block */
typedef struct {
	void *stack;
	void *orig_stack;
	uint8_t in_use;
} tcb_t;

static tcb_t tasks[MAX_TASKS];
static int lastTask;

```

還有用了一大堆組合語言

```cpp
void __attribute__((naked)) thread_start()
{
	lastTask = 0;

	/* Reset APSR before context switch.
	 * Make sure we have a _clean_ PSR for the task.
	 */
	asm volatile("mov r0, #0\n"
	             "msr APSR_nzcvq, r0\n");
	/* To bridge the variable in C and the register in ASM,
	 * move the task's stack pointer address into r0.
	 * http://www.ethernut.de/en/documents/arm-inline-asm.html
	 */
	asm volatile("mov r0, %0\n" : : "r" (tasks[lastTask].stack));
	asm volatile("msr psp, r0\n"
	             "mov r0, #3\n"
	             "msr control, r0\n"
	             "isb\n");
	/* This is how we simulate stack handling that pendsv_handler
	 * does. Thread_create sets 17 entries in stack, and the 9
	 * entries we pop here will be pushed back in pendsv_handler
	 * in the same order.
	 *
	 *
	 *                      pop {r4-r11, lr}
	 *                      ldr r0, [sp]
	 *          stack
	 *  offset -------
	 *        |   16  | <- Reset value of PSR
	 *         -------
	 *        |   15  | <- Task entry
	 *         -------
	 *        |   14  | <- LR for task
	 *         -------
	 *        |  ...  |                             register
	 *         -------                              -------
	 *        |   9   | <- Task argument ---->     |   r0  |
	 * psp after pop--<                             -------
	 *        |   8   | <- EXC_RETURN    ---->     |   lr  |
	 *         -------                              -------
	 *        |   7   |                            |  r11  |
	 *         -------                              -------
	 *        |  ...  |                            |  ...  |
	 *         -------                              -------
	 *        |   0   |                            |   r4  |
	 * psp ->  -------                              -------
	 *
	 * Instead of "pop {r0}", use "ldr r0, [sp]" to ensure consistent
	 * with the way how PendSV saves _old_ context[1].
	 */
	asm volatile("pop {r4-r11, lr}\n"
	             "ldr r0, [sp]\n");
	/* Okay, we are ready to run first task, get address from
	 * stack[15]. We just pop 9 register so #24 comes from
	 * (15 - 9) * sizeof(entry of sp) = 6 * 4.
	 */
	asm volatile("ldr pc, [sp, #24]\n");

	/* Never reach here */
	while(1);
}
```