/* 
 * test_display test should be performed first, because this main also needs
 * the display of course.
 */

#include <string.h>
#include <stdio.h>

#include "ll_input.h"
#include "ll_hwinit_display.h"
#include "display_shared.h"
#include "display.h"
#include "ll_delay.h"

int main(int argc, char** argv)
{
	unsigned char button;
	unsigned char mode;
	unsigned char light;

	struct display_ctx ctx_disp;
	struct display_shared data = {
		.set_brightness = DISPLAY_BRIGHTNESS_PERC_100,
		.num_entries    = 1,
	};

	data.entry_lengths[0] = strlen("B__ M__ L__");
	data.entry_offsets[0] = 0;
	data.entry_x[0]       = 0;
	data.entry_y[0]       = 0;
	data.entry_font[0]    = DISPLAY_FONT_NORMAL;

	ll_input_init();
	ll_hwinit_display();
	display_init_ctx(&ctx_disp);

	while(1) {
		button = ll_input_read_buttons();
		mode   = ll_input_read_mode();
		light  = ll_input_read_sensor();

		sprintf(data.entry_text, "B%02x M%02x L%02x", button, mode,
									light);

		display_update(&ctx_disp, &data);
		ll_delay_ms(200);
	}

	return 0;
}
