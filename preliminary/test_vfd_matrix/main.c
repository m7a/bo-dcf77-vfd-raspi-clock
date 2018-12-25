#include <string.h>
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

static inline void writecmd(char command, unsigned data_len, char* data)
{
	unsigned i;

	/* -- 1 -- */
	PORTB |= _BV(PIN_CONTROL_DATA_INV);

	/* -- 2 -- */
	PORTB &= ~_BV(PIN_SS);

	/* -- 3 -- */
	SPDR = command;
	while(!(SPSR & _BV(SPIF)))
		;

	/* -- 4 -- */
	PORTB |= _BV(PIN_SS);

	if(command == GP9002_CLEARSCREEN) {
		_delay_ms(1);
		return;
	}

	_delay_us(1); /* 400 ns (somewhere between 4 and 6) */

	/* -- 5 -- */
	if(data_len == 0)
		return;

	PORTB &= ~_BV(PIN_CONTROL_DATA_INV);

	/* -- 6 -- */
	PORTB &= ~_BV(PIN_SS);

	/* -- 7, 8 -- */
	for(i = 0; i < data_len; i++) {
		SPDR = data[i];
		
		while(!(SPSR & _BV(SPIF)))
			;

		/* TODO WHY WOULD THIS BE NEEDED ONLY IF DELAY IS NO GOOD AT ALL AND THEN AT LEAST SINGLE WRITE WILL WORK? */
#if 0
		if(i != (i - 1))
			_delay_us(1); /* 600 ns */
#endif
	}

	/* -- 9 -- */
	PORTB |= _BV(PIN_SS);

	_delay_us(1); /* library does this, datasheet says no. */
}

int main()
{
	char buf[5];

	/* -- Init -- */

	/* port directions */
	DDRB = (DDRB & ~_BV(DDB4)) /* IN */
		| _BV(DDB0) | _BV(DDB2) | _BV(DDB3) | _BV(DDB5) /* OUT */;

	/* hardware SPI */
	SPCR =  /* _BV(DORD) | * data order lsb first TODO LIBRARY SAYS MSB FIRST! */
		_BV(SPE)  | /* SPI enable */
		            /* not enabled: SPIE -- spi interrupt enable */
		_BV(MSTR) | /* SPI master mode */
		_BV(CPOL) | /* clock polarity: clock idle at 1 */
		_BV(CPHA) | /* clock phase: sample on leading edge of SCK */
		_BV(SPR0) |
		_BV(SPR1);  /* clock rate select: selct f/128 (library has /4) */

	/* chose /16 instead of /8 for frequency by unsetting SPI2X bit */
	SPSR = _BV(SPI2X); /* TODO NOTE THAT THIS IS CRAZY BUT WORKED BEFORE? */

	/* default high */
	PORTB |= _BV(PIN_SS);

	buf[0] = GP9002_DISPLAY_MONOCHROME;
	writecmd(GP9002_DISPLAY, 1, buf);

	writecmd(GP9002_DISPLAY1ON, 0, NULL);

	_delay_ms(3000);
	buf[0] = 0x12; /* 70% */
	writecmd(GP9002_BRIGHT, 1, buf);

	_delay_ms(5000);
	writecmd(GP9002_CLEARSCREEN, 0, NULL);

	buf[0] = 0x41;
	buf[1] = 0x42;
	buf[2] = 0x43;
	buf[3] = 0x44;
	buf[4] = 0x45;
	writecmd(GP9002_DRAWCHAR, 5, buf);

	_delay_ms(4000);

	writecmd(GP9002_DISPLAYSOFF, 0, NULL);

	while(1)
		;

	return 0;
}
