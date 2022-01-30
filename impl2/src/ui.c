#include <stdio.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"
#include "display_shared.h"
#include "display.h"
#include "ui.h"
#include "formatted_display.h"

void ui_init(struct ui* ctx, struct display_shared* display)
{
	display->set_brightness = DISPLAY_BRIGHTNESS_PERC_100;
	formatted_display_coypright(display);

	ctx->al_h = 24;
	ctx->al_m = 1;
	ctx->out_buzzer_on = 0;
}

void ui_update(struct ui* ctx, struct dcf77_bitlayer* bitlayer,
		struct dcf77_secondlayer* secondlayer,
		struct dcf77_timelayer* timelayer,
		struct display_shared* display, unsigned delay)
{
	char* qos;
	char info[16] = { 0, };

	switch(timelayer->out_qos) {
	case DCF77_TIMELAYER_QOS1:       qos = "+1"; break;
	case DCF77_TIMELAYER_QOS2:       qos = "+2"; break;
	case DCF77_TIMELAYER_QOS3:       qos = "+3"; break;
	case DCF77_TIMELAYER_QOS4:       qos = "o4"; break;
	case DCF77_TIMELAYER_QOS5:       qos = "o5"; break;
	case DCF77_TIMELAYER_QOS6:       qos = "o6"; break;
	case DCF77_TIMELAYER_QOS7:       qos = "-7"; break;
	case DCF77_TIMELAYER_QOS8:       qos = "-8"; break;
	case DCF77_TIMELAYER_QOS9_ASYNC: qos = "-9"; break;
	default:                         qos = "99"; break;
	}

	sprintf(info, "%s%02x%02x%d%d", qos, secondlayer->out_fault_reset,
		bitlayer->out_unidentified, bitlayer->out_reading,
		bitlayer->out_misaligned);
	
	formatted_display_datetime(display, timelayer->out_current.y,
		timelayer->out_current.m, timelayer->out_current.d,
		timelayer->out_current.h, timelayer->out_current.i,
		timelayer->out_current.s, ctx->al_h, ctx->al_m, info);
	formatted_display_debug_activity(display, delay);
}
