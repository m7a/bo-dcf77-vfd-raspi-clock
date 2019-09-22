#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "ll_interrupt.h"
#include "interrupt.h"
#include "inc_sat.h"

static volatile uint32_t      interrupt_time         = 0;
static volatile unsigned char interrupt_is_1         = 0;
static volatile unsigned char interrupt_n1           = 0;
static volatile unsigned char interrupt_n1_out       = 0;
static volatile unsigned char interrupt_ticks_ago    = 0;
static volatile unsigned char interrupt_num_overflow = 0;

void interrupt_service_routine(char pinval)
{
	/* Forward time by 8ms */
	interrupt_time += 8;

	/* Process DCF77 input */
	INC_SATURATED(interrupt_ticks_ago);
	if(pinval) {
		if(!interrupt_is_1) {
			interrupt_n1   = 1;
			interrupt_is_1 = 1;
		} else {
			INC_SATURATED(interrupt_n1);
		}
	} else if(interrupt_is_1) {
		if(interrupt_n1_out != 0)
			INC_SATURATED(interrupt_num_overflow);

		interrupt_is_1      = 0;
		interrupt_ticks_ago = 0;
		interrupt_n1_out    = interrupt_n1;
	}
}

uint32_t interrupt_get_time_ms()
{
	uint32_t val;
	ll_interrupt_handling_disable();
	val = interrupt_time;
	ll_interrupt_handling_enable();
	return val;
}

unsigned char interrupt_get_num_overflow()
{
	return interrupt_num_overflow;
}

/* Procedure to read. Writes to output parameters. No update if val=0 */
void interrupt_read_dcf77_signal(unsigned char* val, unsigned char* ticks_ago)
{
	register unsigned char extracted_n1;
	register unsigned char extracted_ticks_ago;

	/*
	 * Read data by means of atomic Load And Clear instruction.
	 *
	 * If we are interrupted between the two calls, the ISR will notice
	 * this because one of the variables will be non-zero and this will
	 * in turn cause an overflow to be generated.
	 *
	 * This would work were the chip to support it :( :(

	   asm volatile("lac %1, %0": "=r"(extracted_ticks_ago),
						"+z"(interrupt_ticks_ago));

	   asm volatile("lac %1, %0": "=r"(extracted_n1),
						"+z"(interrupt_n1_out));
	 *
	 * As it is not supported, cli()/sei() is needed :(
	 */

	ll_interrupt_handling_disable();
	extracted_n1        = interrupt_n1_out;
	interrupt_n1_out    = 0;
	extracted_ticks_ago = interrupt_ticks_ago;
	ll_interrupt_handling_enable();

	*ticks_ago = extracted_ticks_ago;
	*val       = extracted_n1;
}
