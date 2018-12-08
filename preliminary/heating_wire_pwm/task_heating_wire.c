#include <avr/io.h>
#include <avr/interrupt.h>

/*
 * !!! Aus irgendwelchen Gründen war die gemessene Frequenz 3.9 kHz, eventuell
 *     müsste in der Zeile TCCR2B statt einer 1 eine 3 stehen, sodass
 *     1*2*2 = 4-fach skaliiert wird?
 */
void task_heating_wire_enable()
{
	cli(); /* Interrupts deaktivieren */

	/*
	 * Es sollen 140 mA = 0.14 A fließen. U = 12V, C = 1.5µF.
	 * f = I/UC = 0.14A/(12V * 1.5 * 10**-6 F) = 7777.78 Hz.
	 * lambda = 1/f = 1.28e-4
	 *
	 * Da es Auflade- und Entladezyklen gibt, muss das Signal mit doppelter
	 * Frequenz generiert/invertiert werden, also f = 15555.56 Hz. Mit der
	 * Frequenz des Arduino von 16 * 10**6 Hz ist das also alle
	 * 1028.5714 Zyklen. Timer 2 hat 8 bit (255..0),
	 * also mit prescale = 8 -> alle 129 skalierten Zyklen.
	 */
	TCCR2A = 0;              /* Kein HW-PWM mit Timer 2, da Software-PWM */
	TCNT2  = 0;              /* Zähler initial 0 */
	/* Konfiguriert prescale = 8 und clear timer on compare modus (WGM12) */
	TCCR2B = (1 << CS11) | (1 << WGM12);
	OCR2A  = 129;            /* skalierte Zyklen */
	TIMSK2 |= (1 << OCIE2A); /* Timer Compare Interrupt aktivieren */
	/* Verwende Arduino-Port D5 aka. T1 aka. 9 */
	DDRD |= _BV(DDD5);       /* als Ausgabeport aktivieren */

	sei(); /* Interrupts aktivieren */
}

/* interrupt service routine */
ISR(TIMER2_COMPA_vect)
{
	PORTD ^= _BV(PORTD5); /* Signal invertieren */
}
