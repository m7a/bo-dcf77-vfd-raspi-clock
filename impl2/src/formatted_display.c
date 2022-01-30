#include <string.h>
#include <stdio.h>

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
		"\217"            " DCF'18'22 "                 "\217"
		"\211\216\216\216\216\216\216\216\216\216\216\216\213"
	);
}

/* TODO DEBUG ONLY */
void formatted_display_debug_activity(struct display_shared* dshr,
								unsigned delay)
{
	char* const ACTIVITY_INDICATORS = "/-\\|";
	static char* curact = ACTIVITY_INDICATORS;

	dshr->entry_lengths[dshr->num_entries] = 1;
	dshr->entry_offsets[dshr->num_entries] =
				dshr->entry_offsets[dshr->num_entries - 1] +
				dshr->entry_lengths[dshr->num_entries - 1];
	dshr->entry_text[dshr->entry_offsets[dshr->num_entries]] = *curact;
	dshr->entry_x[dshr->num_entries] = 114;
	dshr->entry_y[dshr->num_entries] = 0;
	dshr->num_entries++;

	dshr->entry_lengths[dshr->num_entries] = 3;
	dshr->entry_offsets[dshr->num_entries] =
				dshr->entry_offsets[dshr->num_entries - 1] +
				dshr->entry_lengths[dshr->num_entries - 1];
	sprintf(dshr->entry_text + dshr->entry_offsets[dshr->num_entries],
		"%03x", delay & 0xfff); /* only lower 3 nibbles */
	dshr->entry_x[dshr->num_entries] = 0;
	dshr->entry_y[dshr->num_entries] = 0;
	dshr->num_entries++;

	if(*(++curact) == 0)
		curact = ACTIVITY_INDICATORS;
}

void formatted_display_datetime(struct display_shared* dshr,
			short y, unsigned char m, unsigned char d,
			unsigned char h, unsigned char i, unsigned char s,
			unsigned char al_h, unsigned char al_m,
			char* info)
{
	dshr->num_entries = 4;

	dshr->entry_lengths[0] = strlen("12:34:56");
	dshr->entry_lengths[1] = strlen("12.34.5678");
	dshr->entry_lengths[2] = strlen("AL09:55");
	dshr->entry_lengths[3] = strlen(info);

	dshr->entry_offsets[0] = 0;
	dshr->entry_offsets[1] = dshr->entry_lengths[0];
	dshr->entry_offsets[2] = dshr->entry_offsets[1] + dshr->entry_lengths[1];
	dshr->entry_offsets[3] = dshr->entry_offsets[2] + dshr->entry_lengths[2];

	dshr->entry_x[0] = 0;
	dshr->entry_x[1] = 30;
	dshr->entry_x[2] = 70;
	dshr->entry_x[3] = 0;

	dshr->entry_y[0] = 16;
	dshr->entry_y[1] = 0;
	dshr->entry_y[2] = 48;
	dshr->entry_y[3] = 48;

	dshr->entry_font[0] = DISPLAY_FONT_LARGE;
	dshr->entry_font[1] = DISPLAY_FONT_NORMAL;
	dshr->entry_font[2] = DISPLAY_FONT_NORMAL;
	dshr->entry_font[3] = DISPLAY_FONT_NORMAL;

	sprintf(dshr->entry_text, "%02u:%02u:%02u""%02u.%02u.%04u"
			"AL%02u:%02u%s", h, i, s, d, m, y, al_h, al_m, info);
}
