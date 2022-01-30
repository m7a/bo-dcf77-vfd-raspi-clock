#include <avr/io.h>

#include "ll_input.h"

#define INPUT_MUX_SENSOR         (_BV(MUX2) |             _BV(MUX0)) /* ADC5 */
#define INPUT_MUX_MODE_SELECTOR  (_BV(MUX2) | _BV(MUX1)            ) /* ADC6 */
#define INPUT_MUX_BUTTONS        (_BV(MUX2) | _BV(MUX1) | _BV(MUX0)) /* ADC7 */

static unsigned char read_val(unsigned mux);

void ll_input_init()
{
	/* Enable ADC */

	/* mux not enabled -> is now at ADC0 (A0 / NC) */
	ADMUX = _BV(REFS0) |  /* AVcc with external capacitor on AREF pin */
		_BV(ADLAR);   /* configure to put much information in ADCH such
				 that we can ingnore ADCL (which would hold more
				 precise information in it's two msb) */
	ADCSRA = _BV(ADEN)  | /* enable */
		 _BV(ADPS2) |
		 _BV(ADPS1) |
		 _BV(ADPS0);  /* 111 -> divide by 128 for "slow" reading */
}

unsigned char ll_input_read_sensor()
{
	return read_val(INPUT_MUX_SENSOR);
}

static unsigned char read_val(unsigned mux)
{
	unsigned char rv;

	ADMUX  |= mux;            /* set mux */
	ADCSRA |= _BV(ADSC);      /* start */
	while(ADCSRA & _BV(ADSC)) /* wait */
		;
	rv = ADCH;                /* store measurement */
	ADMUX &= ~mux;            /* reset mux */
	return rv;
}

unsigned char ll_input_read_buttons()
{
	return read_val(INPUT_MUX_BUTTONS);
}

unsigned char ll_input_read_mode()
{
	return read_val(INPUT_MUX_MODE_SELECTOR);
}
