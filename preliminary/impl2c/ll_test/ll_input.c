#include <stdio.h>

#include "ll_test_acklogic_and_interrupt.h"
#include "ll_input.h"

void ll_input_init()
{
	/* nop */
}

#define LL_INPUT_FUNC(Y) \
	unsigned char Y() \
	{ \
		ll_test_handle_pending_interrupts(); \
		puts(#Y); \
		fflush(stdout); \
		return ll_test_repl_and_interrupt("REPL," #Y ","); \
	}

LL_INPUT_FUNC(ll_input_read_sensor)
LL_INPUT_FUNC(ll_input_read_buttons)
LL_INPUT_FUNC(ll_input_read_mode)
