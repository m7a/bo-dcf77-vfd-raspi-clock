/*
 * Ma_Sys.ma DCF-77 VFD Module Clock 1.0.0, Copyright (c) 2018 Ma_Sys.ma.
 * For further info send an e-mail to Ma_Sys.ma@web.de.
 *
 * I/O
 * ---
 *
 * VFD Connection
 *
 * Arduino     VFD        Other Names  Used Name     Direction Arduino POV
 * D8          14 / C/D'  PB0(ICP)     PORTB0 DDB0   OUT
 * D10 / SS    13 / CS'   PB2(SS)      PORTB2 DDB2   OUT
 * D11 / MOSI  12 / SI    PB3(MOSI)    PORTB3 DDB3   OUT
 * D12 / MISO  8  / SO    PB4          PORTB4 DDB4   IN
 * D13 / SCK   11 / CLK   PB5          PORTB5 DDB5   OUT
 *
 * Sensory Inputs
 *
 * Arduino     HW
 * A7 / ADC7   Taster                                IN / ANALOG
 * A6 / ADC6   Drehknopf                             IN / ANALOG
 * A5 / ADC5   Light Detector                        IN / ANALOG
 *
 * Other I/O
 *
 * D7 / AIN1   Buzzer     PD7                        OUT
 * D2 / IND0   DCF-77     PD2          PORTD2 DDD2   IN / DIGITAL
 */

#include <avr/io.h>
#include <util/delay.h>

#include "vfd_gp9002.h"
#include "input.h"
#include "screen.h"
#include "interrupt.h"

int main()
{
	struct vfd_gp9002 vfd;
	struct input in;
	struct screen scr;

	enum input_mode         in_mode;
	enum input_button_press in_button;

	vfd_gp9002_init(&vfd, VFD_GP9002_FONT_NORMAL);
	input_init(&in);
	screen_init(&scr, &vfd);
	interrupt_enable();

	/* display version */
	screen_update(&scr);
	_delay_ms(2000);

	/* TODO DEBUG ONLY */
	screen_display(&scr, SCREEN_STATUS);

	while(1) {
		in_mode = input_read_mode(&in);
		in_button = input_read_buttons(&in);
		input_read_sensor(&in);

		screen_set_measurements(&scr, in.mode, in.btn, in.sensor,
							in_mode, in_button);

		screen_update(&scr);
		_delay_ms(100);
	}

	/* should never exit */
	return 1;
}
