#include "os.h"

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
		lib_puts("Tryin to get the lock... \n");
		lock_acquire(&lock);
		lib_puts("Get the lock!\n");
		lock_free(&lock);
		lib_delay(1000);
	}
}

void user_init()
{
	lock_init(&lock);
	task_create(&user_task0);
	task_create(&user_task1);
	task_create(&user_task2);
	task_create(&user_task3);
}
