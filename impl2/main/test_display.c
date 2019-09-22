#include <string.h>

#include "ll_hwinit_display.h"
#include "display_shared.h"
#include "display.h"
#include "ll_delay.h"

int main(int argc, char** argv)
{
	unsigned i;
	struct display_ctx ctx_disp;
	struct display_shared data = {
		.set_brightness = DISPLAY_BRIGHTNESS_PERC_100,
		.num_entries    = 3,
	};

	data.entry_lengths[0] = strlen("12:34:56");
	data.entry_lengths[1] = strlen("12.34.5678");
	data.entry_lengths[2] = strlen("AL09:55");

	data.entry_offsets[0] = 0;
	data.entry_offsets[1] = data.entry_lengths[0];
	data.entry_offsets[2] = data.entry_offsets[1] + data.entry_lengths[1];

	data.entry_x[0] = 0;
	data.entry_x[1] = 30;
	data.entry_x[2] = 70;

	data.entry_y[0] = 16;
	data.entry_y[1] = 0;
	data.entry_y[2] = 48;

	data.entry_font[0] = DISPLAY_FONT_LARGE;
	data.entry_font[1] = DISPLAY_FONT_NORMAL;
	data.entry_font[2] = DISPLAY_FONT_NORMAL;

	strcpy(data.entry_text, "12:34:56""12.34.5678""AL09:55");

	ll_hwinit_display();
	display_init_ctx(&ctx_disp);
	display_update(&ctx_disp, &data);

	ll_delay_ms(5000); /* wait 5 sec */

	/* now do an A-Z marquee */
	data.entry_offsets[1] = data.entry_offsets[2] + data.entry_lengths[2];
	strcpy(data.entry_text + data.entry_offsets[1],
		"+++ abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ +++"
		"EEEEEEEEEEEE"
	);

	data.entry_lengths[1] = 10;

	for(i = 0; i < 51; i++) {
		display_update(&ctx_disp, &data);
		ll_delay_ms(250);
		data.entry_offsets[1]++;
	}

	while(1) /* delay forever */
		ll_delay_ms(4000);
	
	return 0;
}
