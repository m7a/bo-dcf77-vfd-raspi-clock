#include <string.h>

#include "input.h"

void input_init(struct input* ctx)
{
	memset(ctx, 0, sizeof(struct input));

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

enum input_mode input_read_mode(struct input* ctx)
{
	ctx->mode = read_val(INPUT_MUX_MODE_SELECTOR);
	return INPUT_MODE_INVALID; /* TODO MEASURE THEN IMPLEMENT */
}

enum input_button_press input_read_btn(struct input* ctx)
{
	ctx->btn = read_val(INPUT_MUX_BUTTONS);

	/*
	 * Button measurement evaluation
	 * 
	 * Measurement  Button          Safety PM  Interval
	 *   0 +- 1     => none         +30        [  0; 30]
	 * 127 +- 1     => 10k / BTN2   6          [121;133]
	 * 194 +- 3     => both         8          [186;202]
	 * 172 +- 3     => 4.7k / BTN1  8          [164;180]
	 */

	if(ctx->btn <= 30)
		return INPUT_BUTTON_NONE;

	if(ctx->btn <= 133 && ctx->btn >= 121)
		return INPUT_BUTTON_2_ONLY;

	if(ctx->btn <= 202 && ctx->btn >= 186)
		return INPUT_BUTTON_BOTH;

	if(ctx->btn <= 180 && ctx->btn >= 164)
		return INPUT_BUTTON_1_ONLY;

	return INPUT_BUTTON_INVALID;
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

/* TODO NEED TO SOMEHOW INTERPRET THE MEASUREMENT RESULT -> should eventually map to display brightness somehow. */
unsigned char input_read_sensor(struct input* ctx)
{
	ctx->sensor = read_val(INPUT_MUX_SENSOR);
	return ctx->sensor;
}
