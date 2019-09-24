#include <string.h>

#include "display_shared.h"
#include "formatted_display.h"

void formatted_display_coypright(struct display_shared* dshr)
{
	dshr->num_entries      =  4;
	dshr->entry_offsets[0] =  0;
	dshr->entry_offsets[1] = 13;
	dshr->entry_offsets[2] = 26;
	dshr->entry_offsets[3] = 39;
	dshr->entry_lengths[0] = 13;
	dshr->entry_lengths[1] = 13;
	dshr->entry_lengths[2] = 13;
	dshr->entry_lengths[3] = 13;
	dshr->entry_x[0]       = 12;
	dshr->entry_x[1]       = 12;
	dshr->entry_x[2]       = 12;
	dshr->entry_x[3]       = 12;
	dshr->entry_font[0]    = DISPLAY_FONT_NORMAL;
	dshr->entry_font[1]    = DISPLAY_FONT_NORMAL;
	dshr->entry_font[2]    = DISPLAY_FONT_NORMAL;
	dshr->entry_font[3]    = DISPLAY_FONT_NORMAL;
	dshr->entry_y[0]       =  0;
	dshr->entry_y[1]       = 16;
	dshr->entry_y[2]       = 32;
	dshr->entry_y[3]       = 48;
	strcpy(
		dshr->entry_text,
		"\212\216\216\216\216\216\216\216\216\216\216\216\214"
		"\217"            " Ma_Sys.ma "                 "\217"
		"\217"            " DCF'18'19 "                 "\217"
		"\211\216\216\216\216\216\216\216\216\216\216\216\213"
	);
}
