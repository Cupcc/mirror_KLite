#include "kernel.h"
#include "bsp.h"
#include "app.h"

static void init(void *arg)
{
	app_init();
}

static void idle(void *arg)
{
	kernel_idle();
}

void heap_overflow(void)
{
	while(1);
}

int main(void)
{
	static uint8_t heap[64*1024];
	bsp_init();
	kernel_init(heap, sizeof(heap));
	thread_create(idle, NULL, 0);
	thread_create(init, NULL, 0);
	kernel_start();
}
