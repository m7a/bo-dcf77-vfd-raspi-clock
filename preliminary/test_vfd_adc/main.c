#include <stdio.h>
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
 *
 * Arduino     HW
 * A7 / ADC7   Taster                                IN / ANALOG
 * A6 / ADC6   Drehknopf                             IN / ANALOG
 * A5 / ADC5   Sensor                                IN / ANALOG
 */

#define PIN_CONTROL_DATA_INV PORTB0
#define PIN_SS               PORTB2

static void write(char is_ctrl, char value)
{
	PORTB = (PORTB & ~_BV(PIN_CONTROL_DATA_INV)) |
					(is_ctrl << PIN_CONTROL_DATA_INV);

	PORTB &= ~_BV(PIN_SS);

	SPDR = value;
	while(!(SPSR & _BV(SPIF)))
		;

	PORTB |= _BV(PIN_SS);
	_delay_us(1);

	if(is_ctrl && value == GP9002_CLEARSCREEN)
		_delay_us(270);
}

int main()
{
	char result[25];

	unsigned char val_taster;
	unsigned char val_knopf;
	unsigned char val_sensor;

	unsigned char round;
	unsigned char symbol;

	int i;
	int n;

	char ctrl_data[][2] = {

		/* set display mode */
		{ 1, GP9002_DISPLAY },
		{ 0, GP9002_DISPLAY_MONOCHROME },

		/* set weak brightness */
		{ 1, GP9002_BRIGHT },
		{ 0, 0x2a }, /* 40% */

		{ 1, GP9002_CLEARSCREEN },

		{ 1, GP9002_DISPLAY1ON },

		{ 1, GP9002_DRAWCHAR },
		{ 0, 'A' },
		{ 0, 'B' },

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
	for(i = 0; i < n; i++)
		write(ctrl_data[i][0], ctrl_data[i][1]);

	/* -- ADC -- */

	/* mux not enabled -> is now at ADC0 (A0 / NC) */
	ADMUX = _BV(REFS0) | /* AVcc with external capacitor on AREF pin */
		_BV(ADLAR);  /* configure to put much information in ADCH such
				that we can ingnore ADCL (which would hold more
				precise information in it's two msb) */
	ADCSRA = _BV(ADEN)  | /* enable */
		 _BV(ADPS2) |
		 _BV(ADPS1) |
		 _BV(ADPS0); /* 111 -> divide by 128 for "slow" reading */

	round = 0;

	while(1) {
		/* ADC7 / Taster -> MUX2, MUX1, MUX0 */
		ADMUX  |= _BV(MUX2) | _BV(MUX1) | _BV(MUX0);
		ADCSRA |= _BV(ADSC);      /* start */
		while(ADCSRA & _BV(ADSC)) /* wait */
			;
		val_taster = ADCH;        /* read value */
		ADMUX &= ~(_BV(MUX2) | _BV(MUX1) | _BV(MUX0));

		/* ADC6 / Drehknopf -> MUX 2, MUX 1 */
		ADMUX  |= _BV(MUX2) | _BV(MUX1);
		ADCSRA |= _BV(ADSC);      /* start */
		while(ADCSRA & _BV(ADSC)) /* wait */
			;
		val_knopf = ADCH;
		ADMUX &= ~(_BV(MUX2) | _BV(MUX1));

		/* ADC5 / Sensor -> MUX2, MUX0 */
		ADMUX  |= _BV(MUX2) | _BV(MUX0);
		ADCSRA |= _BV(ADSC);      /* start */
		while(ADCSRA & _BV(ADSC)) /* wait */
			;
		val_sensor = ADCH;
		ADMUX &= ~(_BV(MUX2) | _BV(MUX0));

		if((round++ % 20) >= 10)
			symbol = '|';
		else
			symbol = '-';

		n = snprintf(result, sizeof(result), "7:%3u 6:%3u 5:%3u %c",
				val_taster, val_knopf, val_sensor, symbol);
		write(1, GP9002_CLEARSCREEN);
		write(1, GP9002_CHARRAM);
		write(0, 0);
		write(0, 0);
		write(0, 0);
		write(1, GP9002_DRAWCHAR);
		for(i = 0; i < n; i++)
			write(0, result[i]);

		_delay_ms(100);
		round++;
	}

	return 0;
}
