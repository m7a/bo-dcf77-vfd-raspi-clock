#include "interrupt.h"

/* TODO ... NEED SOME OTHER DECLARATIONS ETC. */

static volatile char interrupt_triggered;

void interrupt_enable()
{
	return; /* TODO DEBUG ONLY DISABLED FOR SAFETY REASONS */

	cli(); /* asm("cli") */

	/* switch to IN direction */
	DDRD &= ~_BV(INTERRUPT_USE_PIN_DD);

	/* Configure interrupts to trigger on exactly a rising edge of INT 0 */
	EICRA = _BV(ISC01) | _BV(ISC00);
	EIMSK = _BV(INT0);

	/* read by EIFR & _BV(INTF0) */

	sei();

	/* TODO CSTAT CONTINUE W/ EXTERNAL INTERRUPT... */
}

ISR(INT0_vect)
{
	/* Zeitmessung... gar nicht so einfach, läuft darauf hinaus einen freien timer zu finden -> es scheint man muss einen interrupt benutzen, um eine Art millis() funktion zu realisieren... */
}

enum interrupt_dcf77_reading interrupt_read()
{
}
