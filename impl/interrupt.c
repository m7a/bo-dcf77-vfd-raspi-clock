#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "interrupt.h"

/* TODO ... NEED SOME OTHER DECLARATIONS ETC. */

static volatile char     interrupt_triggered = 0;
static volatile uint32_t interrupt_last      = 0;
static volatile uint32_t interrupt_delta     = 0;

static volatile uint32_t interrupt_time      = 0;

void interrupt_enable()
{
	cli(); /* asm("cli") */

	/* -- Input Interrupts -- */

	/* switch to IN direction */
	DDRD &= ~_BV(INTERRUPT_USE_PIN_DD);

	/* -- XOND ONLY -- */
	/* Configure interrupts to trigger on exactly a rising edge of INT 0 */
	/* TODO z might not be what one wants. Deactivated for now */
	/* EICRA |= _BV(ISC01) | _BV(ISC00); */
	/* -- END -- */

	EICRA = _BV(ISC00); /* generate interrupt on any logic change */
	EIMSK = _BV(INT0);

	/* read by EIFR & _BV(INTF0) */

	/* -- Timing Interrupt -- */

	TCCR0A = _BV(WGM01);            /* timer 0 mode CTC */
	TCCR0B = _BV(CS02) | _BV(CS00); /* set Clock/1024 prescaler */
	OCR0A  = 125;                   /* count to 125 (125 times: 0--124) */
	TIMSK0 = _BV(OCIE0A);           /* Enable output compare interrupt */

	sei();
}

ISR(INT0_vect)
{
	interrupt_delta     = interrupt_time - interrupt_last;
	interrupt_triggered = !interrupt_triggered;
	interrupt_last      = interrupt_time;
}

enum interrupt_dcf77_reading interrupt_read()
{
	return INTERRUPT_DCF77_READING_NOTHING;
}

ISR(TIMER0_COMPA_vect)
{
	interrupt_time += 8;
}

uint32_t interrupt_get_time_ms()
{
	uint32_t val;
	cli();
	val = interrupt_time;
	sei();
	return val;
}

/* @return 0 if no value available */
uint32_t interrupt_get_delta()
{
	uint32_t rv = 0;
	cli();
	if(interrupt_delta != 0)  {
		rv = interrupt_delta;
		interrupt_delta = 0;
	}
	sei();
	return rv;
}
