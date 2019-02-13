/*
 * Ma_Sys.ma DCF-77 VFD Module Clock 1.0.0, Copyright (c) 2018, 2019 Ma_Sys.ma.
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
 * D7 / AIN1   Buzzer     PD7          PORTD7 DDD7   OUT
 * D2 / IND0   DCF-77     PD2          PORTD2 DDD2   IN / DIGITAL
 */

#include <stddef.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "vfd_gp9002.h"
#include "input.h"
#include "screen.h"
#include "interrupt.h"
#include "dcf77_low_level.h"
#include "alarm.h"

#define DELAY_MS_TARGET   100
#define DELAY_MS_VARIANCE  10

extern char last_reading; /* TODO DEBUG ONLY */

int main()
{
	int delay_ms = DELAY_MS_TARGET;
	int i;

	char debug_counter = 0;
	char debug_info[64];

	uint32_t time_old = 0;
	uint32_t time_new;
	int delta_t;
	size_t debug_info_len = 0;
	char debug_alarm = 0;

	struct vfd_gp9002 vfd;
	struct input in;
	struct screen scr;
	struct dcf77_low_level dcflow;

	enum input_mode         in_mode;
	enum input_button_press in_button;

	vfd_gp9002_init(&vfd, VFD_GP9002_FONT_NORMAL);
	alarm_init();
	input_init(&in);
	screen_init(&scr, &vfd);
	dcf77_low_level_init(&dcflow);
	interrupt_enable();

	/* display version */
	screen_update(&scr);
	_delay_ms(2000);

	/* TODO DEBUG ONLY */
	screen_display(&scr, SCREEN_STATUS);

	while(1) {
		/* == Read Sensors == */
		in_mode = input_read_mode(&in);
		in_button = input_read_buttons(&in);
		input_read_sensor(&in);
		dcf77_low_level_proc(&dcflow); /* TODO z RV ignored */

		time_new = interrupt_get_time_ms();
		delta_t = time_new - time_old;

		/* == Process == */
		debug_info_len = sprintf(debug_info, "%4d %-5s %u %03d %02x",
			delta_t, dcflow.debug, last_reading, delay_ms,
			interrupt_get_num_overflow());
		screen_set_measurements(&scr, in.mode, in.btn, in.sensor,
				in_mode, in_button, debug_info_len, debug_info,
				interrupt_get_time_ms());

		screen_update(&scr);

		/* == Delay == */
		time_old = time_new;

		if(!(DELAY_MS_TARGET - DELAY_MS_VARIANCE <= delta_t &&
				delta_t <= DELAY_MS_TARGET + DELAY_MS_VARIANCE))
			/* /3 -- do not change too rapidly */
			delay_ms += (DELAY_MS_TARGET - delta_t) / 3;

		if(delay_ms < 0)
			delay_ms = 0;

		/* https://www.avrfreaks.net/forum/how-use-delay-variable */
		for(i = 0; i < delay_ms; i++)
			_delay_ms(1);

		/* == TODO DEBUG ONLY == */
		if(++debug_counter == 100) {
			if(debug_alarm) {
				alarm_disable();
				debug_alarm = 0;
			} else {
				alarm_enable();
				debug_alarm = 1;
			}
			debug_counter = 0;
		}
	}

	/* should never exit */
	return 1;
}
