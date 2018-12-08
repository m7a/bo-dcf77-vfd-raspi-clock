#include <avr/io.h>
#include <util/delay.h>

#define BLINK_DELAY_MS 2000

void main()
{
	/* PB5(SCK) = 17 = D13 = LED3 */

	/* set pin 5 of PORTB for output*/
	DDRB |= _BV(DDB5);

	while(1) {
		/*
		 * _BV: Bit mask for port: /usr/lib/avr/include/avr/iom328p.h
		 *                         /usr/lib/avr/include/avr/sfr_defs.h
		 */

		/* set pin 5 high to turn led on */
		PORTB |= _BV(PORTB5);
		_delay_ms(BLINK_DELAY_MS);

		/* set pin 5 low to turn led off */
		PORTB &= ~_BV(PORTB5);
		_delay_ms(BLINK_DELAY_MS);
	}
}
