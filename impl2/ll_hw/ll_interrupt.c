#include <avr/io.h>
#include <avr/interrupt.h>

#include "ll_interrupt.h"
#include "interrupt.h"

/* Pins to read DCF-77 signal from */
#define INTERRUPT_USE_PIN_DD   DDD2
#define INTERRUPT_USE_PIN_READ (PIND & _BV(PD2))

void ll_interrupt_enable()
{
	/* switch to IN direction */
	DDRD &= ~_BV(INTERRUPT_USE_PIN_DD);

	/* -- Timing Interrupt -- */
	cli();                          /* asm("cli") */
	TCCR0A = _BV(WGM01);            /* timer 0 mode CTC */
	TCCR0B = _BV(CS02) | _BV(CS00); /* set Clock/1024 prescaler */
	OCR0A  = 125;                   /* count to 125 (125 times: 0--124) */
	TIMSK0 = _BV(OCIE0A);           /* Enable output compare interrupt */
	sei();
}

ISR(TIMER0_COMPA_vect)
{
	interrupt_service_routine();
}

void ll_interrupt_handling_disable()
{
	cli();
}

void ll_interrupt_handling_enable()
{
	sei();
}

char ll_interrupt_read_pin()
{
	return INTERRUPT_USE_PIN_READ;
}
