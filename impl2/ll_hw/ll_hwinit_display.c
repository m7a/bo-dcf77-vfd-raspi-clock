#include <avr/io.h>

#include "ll_display_shared.h"
#include "ll_display.h"
#include "ll_hwinit_display.h"

void ll_hwinit_display()
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
