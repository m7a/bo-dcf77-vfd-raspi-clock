#include <avr/io.h>

#include "ll_out_buzzer.h"

void ll_out_buzzer_init()
{
	DDRD |= _BV(DDD7); /* PD7 OUT mode */
	ll_out_buzzer(0);
}

void ll_out_buzzer(char on)
{
	if(on)
		PORTD |= _BV(PORTD7);  /* set to 1 */
	else
		PORTD &= ~_BV(PORTD7); /* set to 0 */
}
