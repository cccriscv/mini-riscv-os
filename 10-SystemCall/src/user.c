#include "os.h"
#include "user_api.h"

int shared_var = 500;

lock_t lock;

void user_task0(void)
{
	lib_puts("Task0: Created!\n");
	while (1)
	{
		lib_puts("Task0: Running...\n");
		lib_delay(1000);
	}
}

void user_task1(void)
{
	lib_puts("Task1: Created!\n");
	while (1)
	{
		lib_puts("Task1: Running...\n");
		lib_delay(1000);
	}
}

void user_task2(void)
{
	lib_puts("Task2: Created!\n");
	while (1)
	{
		for (int i = 0; i < 50; i++)
		{
			lock_acquire(&lock);
			shared_var++;
			lock_free(&lock);
			lib_delay(100);
		}
		lib_printf("The value of shared_var is: %d \n", shared_var);
	}
}

void user_task3(void)
{
	lib_puts("Task3: Created!\n");
	while (1)
	{
		lib_puts("Trying to get the lock... \n");
		lock_acquire(&lock);
		lib_puts("Get the lock!\n");
		lock_free(&lock);
		lib_delay(1000);
	}
}

void user_task4(void)
{
	lib_puts("Task4: Created!\n");
	unsigned int hid = -1;
	
	/*
	 * if syscall is supported, this will trigger exception, 
	 * code = 2 (Illegal instruction)
	 */
	// hid = r_mhartid();
	// lib_printf("hart id is %d\n", hid);
	
	// perform system call from the user mode
	int ret = -1;
	ret = gethid(&hid);
        // ret = gethid(NULL);
        if (!ret) {
		lib_printf("system call returned!, hart id is %d\n", hid);
        } else {
                lib_printf("gethid() failed, return: %d\n", ret);
	}

	while (1)
	{	
		lib_puts("Task4: Running...\n");
		lib_delay(1000);
	}
}

void user_init()
{
	task_create(&user_task0);
	task_create(&user_task4);
	// task_create(&user_task1);
}
