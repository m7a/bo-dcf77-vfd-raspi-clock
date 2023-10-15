/* 
 * test_display test should be performed first, because this main also needs
 * the display of course.
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "ll_input.h"
#include "ll_out_display.h"
#include "display_shared.h"
#include "display.h"
#include "interrupt.h"
#include "ll_delay.h"

int main(int argc, char** argv)
{
	unsigned char button;
	unsigned char mode;
	unsigned char light;
	unsigned char val;
	unsigned char ticks_ago;

	struct display_ctx ctx_disp;
	struct display_shared data = {
		.set_brightness = DISPLAY_BRIGHTNESS_PERC_100,
		.num_entries    = 3,
	};

	data.entry_lengths[0] = strlen("B__ M__ L__");
	data.entry_lengths[1] = strlen("D___ A___");
	data.entry_lengths[2] = strlen("T__________");
	data.entry_offsets[0] = 0;
	data.entry_offsets[1] = data.entry_lengths[0];
	data.entry_offsets[2] = data.entry_offsets[1] + data.entry_lengths[1];
	data.entry_x[0]       = 0;
	data.entry_x[1]       = 0;
	data.entry_x[2]       = 0;
	data.entry_y[0]       = 0;
	data.entry_y[1]       = 16;
	data.entry_y[2]       = 32;
	data.entry_font[0]    = DISPLAY_FONT_NORMAL;
	data.entry_font[1]    = DISPLAY_FONT_NORMAL;
	data.entry_font[2]    = DISPLAY_FONT_NORMAL;

	ll_input_init();
	ll_out_display_init();
	display_init_ctx(&ctx_disp);

	while(1) {
		button = ll_input_read_buttons();
		mode   = ll_input_read_mode();
		light  = ll_input_read_sensor();
		interrupt_read_dcf77_signal(&val, &ticks_ago);

		sprintf(
			data.entry_text,
			"B%02x M%02x L%02x"
			"D%03d A%03d"
			"T%10lu",
			button, mode, light, val, ticks_ago,			
			interrupt_get_time_ms()
		);
		display_update(&ctx_disp, &data);
		ll_delay_ms(200);
	}

	return 0;
}
