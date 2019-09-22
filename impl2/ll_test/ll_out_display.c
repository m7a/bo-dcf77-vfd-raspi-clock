#include <stdio.h>

#include "ll_test_acklogic_and_interrupt.h"
#include "ll_display.h"
#include "ll_out_display.h"

void ll_out_display(char is_ctrl, unsigned char value)
{
	ll_test_handle_pending_interrupts();
	printf("ll_out_display,%d,%u\n", is_ctrl, value);
	fflush(stdout);
	ll_test_acklogic_and_interrupt("ACK,ll_out_display\n");
}
