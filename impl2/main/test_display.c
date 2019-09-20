#include <string.h>

#include "ll_hwinit_display.h"
#include "display_shared.h"
#include "display.h"
#include "ll_delay.h"

int main(int argc, char** argv)
{
	struct display_ctx ctx_disp;
	struct display_shared data = {
		.set_brightness = DISPLAY_BRIGHTNESS_PERC_100,
		.num_entries    = 3,
	};


	data.entry_lengths[0] = strlen(">12:34:56<");
	data.entry_lengths[1] = strlen("12.34.5678");
	data.entry_lengths[2] = strlen("AL09:55");

	data.entry_offsets[0] = 0;
	data.entry_offsets[1] = data.entry_lengths[0];
	data.entry_offsets[2] = data.entry_offsets[0] + data.entry_lengths[1];

	data.entry_x[0] = 10;
	data.entry_x[1] = 30;
	data.entry_x[2] = 70;

	data.entry_y[0] = 8;
	data.entry_y[1] = 0;
	data.entry_y[2] = 40;

	strcpy(data.entry_text, ">12:34:56<""12.34.5678""AL09:55");

	ll_hwinit_display();
	display_init_ctx(&ctx_disp);
	display_update(&ctx_disp, &data);

	while(1)
		ll_delay_ms(400000);
	return 0;
}
