#include <stdio.h>

#include "ll_test_acklogic_and_interrupt.h"
#include "ll_delay.h"

void ll_delay_ms(unsigned num)
{
	printf("ll_delay_ms,%u\n", num);
	fflush(stdout);
	ll_test_acklogic_and_interrupt("ACK,ll_delay_ms\n");
}
