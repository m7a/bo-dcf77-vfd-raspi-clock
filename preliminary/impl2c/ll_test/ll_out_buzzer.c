#include <stdio.h>

#include "ll_test_acklogic_and_interrupt.h"
#include "ll_out_buzzer.h"

void ll_out_buzzer_init()
{
	ll_out_buzzer(0);
}

void ll_out_buzzer(char on)
{
	ll_test_handle_pending_interrupts();
	printf("ll_out_buzzer,%d\n", on);
	fflush(stdout);
	ll_test_acklogic_and_interrupt("ACK,ll_out_buzzer\n");
}
