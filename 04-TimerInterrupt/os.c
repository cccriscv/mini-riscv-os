#include "os.h"

void os_start() {
	lib_puts("OS start\n");
	// user_init();
	timer_init(); // start timer interrupt ...
}

int os_main(void)
{
	os_start();
	while (1) {} // stop here !
	return 0;
}

