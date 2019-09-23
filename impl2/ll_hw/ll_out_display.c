#include <avr/io.h>

#include "ll_out_display.h"

/* Uses D10/SS, D11/MOSI, D12/MISO, D13/SCK and the following ports */
#define VFD_GP9002_PIN_CONTROL_DATA_INV PORTB0 /* D8  */
#define VFD_GP9002_PIN_SS               PORTB2 /* D10 */

void ll_out_display_init()
{
	/* port directions */
	DDRB = (DDRB & ~_BV(DDB4)) /* IN */
		| _BV(DDB0) | _BV(DDB2) | _BV(DDB3) | _BV(DDB5) /* OUT */;

	/* hardware SPI */
	SPCR =  _BV(DORD) | /* data order lsb first */
		_BV(SPE)  | /* SPI enable */
		            /* not enabled: SPIE -- spi interrupt enable */
		_BV(MSTR) | /* SPI master mode */
		_BV(CPOL) | /* clock polarity: clock idle at 1 */
		_BV(CPHA) | /* clock phase: sample on leading edge of SCK */
		_BV(SPR0);  /* clock rate select: select f/16  */

	/* Uses /16 instead of /8 for frequency by unsetting SPI2X bit */
	SPSR &= ~_BV(SPI2X);

	PORTB |= _BV(VFD_GP9002_PIN_SS);
}

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
