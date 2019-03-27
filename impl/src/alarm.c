#include <avr/io.h>

#include "alarm.h"

void alarm_init()
{
	DDRD |= _BV(DDD7);     /* PD7 OUT mode */
	alarm_disable();
}

void alarm_enable()
{
	PORTD |= _BV(PORTD7);  /* set to 1 */
}

void alarm_disable()
{
	PORTD &= ~_BV(PORTD7); /* set to 0 */
}
