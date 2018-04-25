#include <stdio.h>
#include "contty.h"

static int foo = 42;

void pcboot_main(void)
{
	con_init();

	printf("hello world: %d\n", foo);
}
