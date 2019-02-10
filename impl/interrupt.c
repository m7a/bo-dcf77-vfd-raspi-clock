#include <string.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "interrupt.h"

/* TODO ... NEED SOME OTHER DECLARATIONS ETC. */

#define SIZE_BYTES

static volatile uint32_t      interrupt_time         = 0;
static volatile unsigned char interrupt_readings[SIZE_BYTES];
static volatile unsigned char interrupt_start        = 0;
static volatile unsigned char interrupt_next         = 0;
static volatile unsigned char interrupt_num_overflow = 0;

void interrupt_enable()
{
	memset(interrupt_readings, 0, sizeof(interrupt_readings));

	/* switch to IN direction */
	DDRD &= ~_BV(INTERRUPT_USE_PIN_DD);

	/* -- Timing Interrupt -- */
	cli(); /* asm("cli") */
	TCCR0A = _BV(WGM01);            /* timer 0 mode CTC */
	TCCR0B = _BV(CS02) | _BV(CS00); /* set Clock/1024 prescaler */
	OCR0A  = 125;                   /* count to 125 (125 times: 0--124) */
	TIMSK0 = _BV(OCIE0A);           /* Enable output compare interrupt */
	sei();
}

ISR(TIMER0_COMPA_vect)
{
	unsigned char idxh = interrupt_next >> 3;
	unsigned char idxl = interrupt_next & 7;

	interrupt_time += 8;

	interrupt_readings[idxh] = (interrupt_readings[idxh] & ~_BV(idxl)) |
					((INTERRUPT_USE_PIN_READ) << idxl);
	if(++interrupt_next == start)
		interrupt_num_overflow = ((interrupt_num_overflow == 0xff)?
					0xff: (interrupt_num_overflow + 1));
}

uint32_t interrupt_get_time_ms()
{
	uint32_t val;
	cli();
	val = interrupt_time;
	sei();
	return val;
}

unsigned char interrupt_get_num_overflow()
{
	return interrupt_num_overflow;
}

unsigned char interrupt_get_start()
{
	return interrupt_start;
}

unsigned char interrupt_set_start(unsigned char start)
{
	interrupt_start = start;
}

unsigned char interrupt_get_next()
{
	return interrupt_next;
}

unsigned char interrupt_get_num_meas()
{
	return interrupt_get_num_between(interrupt_start, interrupt_next);
}

unsigned char intterupt_get_num_between(unsigned char start, unsigned char next)
{
	/* next > start || ... start --- next  ... || next - start        */
	/* start > next || --- next  ... start --- || size - (start-next) */
	return next >= start? (start - next): ((SIZE_BYTES * 8) -
								(start - next));
}

unsigned char interrupt_get_at(unsigned char idx)
{
	return interrupt_readings[idx >> 3] & idx;
}
