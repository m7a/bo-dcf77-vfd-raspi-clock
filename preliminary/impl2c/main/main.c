/*
 * Ma_Sys.ma DCF-77 VFD Module Clock 1.0.0, Copyright (c) 2018-2021 Ma_Sys.ma.
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
#include <stdint.h>

#include "ll_out_buzzer.h"
#include "ll_out_display.h"
#include "ll_interrupt.h"
#include "ll_input.h"
#include "ll_delay.h"

#include "display_shared.h"
#include "display.h"
#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"
#include "ui.h"
#include "mainloop_timing.h"
#include "interrupt.h"

int main()
{
	struct display_ctx         display_private;
	struct display_shared      display;
	struct dcf77_bitlayer      dcf77_bitlayer;
	struct dcf77_secondlayer   dcf77_secondlayer;
	struct dcf77_timelayer     dcf77_timelayer;
	struct ui                  ui;
	struct mainloop_timing_ctx mainloop_timing;

	unsigned delay = 0;

	ll_input_init();
	ll_out_display_init();
	ll_out_buzzer_init();

	display_init_ctx(&display_private);
	ui_init(&ui, &display);
	display_update(&display_private, &display);

	dcf77_bitlayer_init(&dcf77_bitlayer),
	dcf77_secondlayer_init(&dcf77_secondlayer);
	dcf77_timelayer_init(&dcf77_timelayer);
	mainloop_timing_init(&mainloop_timing);

	ll_delay_ms(2000); /* wait 2 sec */

	ll_interrupt_enable();
	while(1) {
		/* == Read DCF-77 Signal, Process DCF-77 Bitlayer == */
		interrupt_read_dcf77_signal(&dcf77_bitlayer.in_val,
						&dcf77_bitlayer.in_ticks_ago);
		dcf77_bitlayer_proc(&dcf77_bitlayer);
		mainloop_timing_pre(&mainloop_timing, interrupt_get_time_ms());
		if(dcf77_bitlayer.out_misaligned) /* Add delay if misaligned */
			ll_delay_ms(25);

		/* == Process Data == */
		dcf77_secondlayer.in_val = dcf77_bitlayer.out_reading;
		dcf77_secondlayer_process(&dcf77_secondlayer);
		dcf77_timelayer_process(&dcf77_timelayer,
			dcf77_bitlayer.out_reading != DCF77_BIT_NO_UPDATE,
			&dcf77_secondlayer);

		/* == Read Sensors + Process User Interface == */
		ui.in_sensor  = ll_input_read_sensor();
		ui.in_buttons = ll_input_read_buttons();
		ui.in_mode    = ll_input_read_mode();
		ui_update(&ui, &dcf77_bitlayer, &dcf77_secondlayer,
					&dcf77_timelayer, &display, delay);

		/* == Update Outputs == */
		ll_out_buzzer(ui.out_buzzer_on);
		display_update(&display_private, &display);

		/* == Delay before next Iteration == */
		delay = mainloop_timing_post_get_delay(&mainloop_timing);
		ll_delay_ms(delay);
	}
	
	return 0; /* this should never happen */
}
