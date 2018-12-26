#include <avr/io.h>
#include <util/delay.h>

#include "z_gp9002_const.h"

/*
 * I/O
 *
 * Arduino     VFD        Other Names  Used Name     Direction Arduino POV
 * D8          14 / C/D'  PB0(ICP)     PORTB0 DDB0   OUT
 * D10 / SS    13 / CS'   PB2(SS)      PORTB2 DDB2   OUT
 * D11 / MOSI  12 / SI    PB3(MOSI)    PORTB3 DDB3   OUT
 * D12 / MISO  8  / SO    PB4          PORTB4 DDB4   IN
 * D13 / SCK   11 / CLK   PB5          PORTB5 DDB5   OUT
 */

#define PIN_CONTROL_DATA_INV PORTB0
#define PIN_SS               PORTB2

int main()
{
	int i;
	int n;
	char ctrl_data[][2] = {

		/* set display mode */
		{ 1, GP9002_DISPLAY },
		{ 0, GP9002_DISPLAY_MONOCHROME },

		/* set weak brightness */
		{ 1, GP9002_BRIGHT },
		{ 0, 0x24 }, /* 40% */

		{ 1, GP9002_CLEARSCREEN },

		{ 1, GP9002_DISPLAY1ON },

		{ 1, GP9002_ADDRL },
		{ 0, 0x00 },
		{ 1, GP9002_ADDRH },
		{ 0, 0x00 },

		{ 1, GP9002_DATAWRITE },
		{ 0, 0xff },

	};

	/* -- Init -- */

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
		_BV(SPR1) | /* clock rate select: selct f/128 (slowest) */
		_BV(SPR0);  

	/* chose /128 instead of /64 for frequency by unsetting SPI2X bit */
	SPSR &= ~_BV(SPI2X);

	PORTB |= _BV(PIN_SS);

	n = sizeof(ctrl_data) / (2 * sizeof(char));
	for(i = 0; i < n; i++) {
		PORTB = (PORTB & ~_BV(PIN_CONTROL_DATA_INV)) |
				(ctrl_data[i][0] << PIN_CONTROL_DATA_INV);

		PORTB &= ~_BV(PIN_SS);

		SPDR = ctrl_data[i][1];
		while(!(SPSR & _BV(SPIF)))
			;

		PORTB |= _BV(PIN_SS);
		_delay_us(270);

/*
		if(ctrl_data[i][0] && ctrl_data[i][1] == GP9002_CLEARSCREEN)
			_delay_us(270);
*/
	}

	while(1)
		;

	return 0;
}
