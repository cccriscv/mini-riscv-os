# FreeRTOS

* https://github.com/illustris/FreeRTOS-RISCV/blob/master/Source/portable/GCC/RISCV/port.c

```cpp
/*-----------------------------------------------------------*/

void vPortSysTickHandler( void )
{
	prvSetNextTimerInterrupt();

	/* Increment the RTOS tick. */
	if( xTaskIncrementTick() != pdFALSE )
	{
		vTaskSwitchContext();
	}
}
/*-----------------------------------------------------------*/
```

* https://github.com/illustris/FreeRTOS-RISCV/blob/master/Source/portable/GCC/RISCV/portasm.S

```s
vPortYield:
	/*
	*  This routine can be called from outside of interrupt handler. This means
	*  interrupts may be enabled at this point. This is probably okay for registers and
	*  stack. However, "mepc" will be overwritten by the interrupt handler if a timer
	*  interrupt happens during the yield. To avoid this, prevent interrupts before starting.
	*  The write to mstatus in the restore context routine will enable interrupts after the
	*  mret. A more fine-grain lock may be possible.
	*/  
	csrci mstatus, 8

	portSAVE_CONTEXT
	portSAVE_RA
	jal	vTaskSwitchContext
	portRESTORE_CONTEXT
```

* https://github.com/illustris/FreeRTOS-RISCV/blob/master/Source/tasks.c

```cpp
void vTaskSwitchContext( void )
{
	if( uxSchedulerSuspended != ( UBaseType_t ) pdFALSE )
	{
		/* The scheduler is currently suspended - do not allow a context
		switch. */
		xYieldPending = pdTRUE;
	}
	else
	{
		xYieldPending = pdFALSE;
		traceTASK_SWITCHED_OUT();

		#if ( configGENERATE_RUN_TIME_STATS == 1 )
		{
				#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
					portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime );
				#else
					ulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
				#endif

				/* Add the amount of time the task has been running to the
				accumulated	time so far.  The time the task started running was
				stored in ulTaskSwitchedInTime.  Note that there is no overflow
				protection here	so count values are only valid until the timer
				overflows.  The guard against negative values is to protect
				against suspect run time stat counter implementations - which
				are provided by the application, not the kernel. */
				if( ulTotalRunTime > ulTaskSwitchedInTime )
				{
					pxCurrentTCB->ulRunTimeCounter += ( ulTotalRunTime - ulTaskSwitchedInTime );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
				ulTaskSwitchedInTime = ulTotalRunTime;
		}
		#endif /* configGENERATE_RUN_TIME_STATS */

		/* Check for stack overflow, if configured. */
		taskCHECK_FOR_STACK_OVERFLOW();

		/* Select a new task to run using either the generic C or port
		optimised asm code. */
		taskSELECT_HIGHEST_PRIORITY_TASK();
		traceTASK_SWITCHED_IN();

		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{
			/* Switch Newlib's _impure_ptr variable to point to the _reent
			structure specific to this task. */
			_impure_ptr = &( pxCurrentTCB->xNewLib_reent );
		}
		#endif /* configUSE_NEWLIB_REENTRANT */
	}
}
```
