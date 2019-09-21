#include <avr/io.h>

#include "ll_display.h"
#include "ll_display_pins.h"
#include "ll_out_display.h"

void ll_out_display(char is_ctrl, unsigned char value)
{
	PORTB = (PORTB & ~_BV(VFD_GP9002_PIN_CONTROL_DATA_INV)) |
				(is_ctrl << VFD_GP9002_PIN_CONTROL_DATA_INV);

	PORTB &= ~_BV(VFD_GP9002_PIN_SS);

	SPDR = value;
	while(!(SPSR & _BV(SPIF)))
		;

	PORTB |= _BV(VFD_GP9002_PIN_SS);
	_delay_us(1);

	if(is_ctrl && value == GP9002_CLEARSCREEN)
		_delay_us(270);
}
