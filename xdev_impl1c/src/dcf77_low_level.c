#include <stdint.h>
#include <string.h>

#include "dcf77_low_level.h"
#include "inc_sat.h"

void dcf77_low_level_init(struct dcf77_low_level* ctx)
{
	ctx->private_intervals_of_100ms_passed = 0;
	ctx->in_val                            = 0;
	ctx->in_ticks_ago                      = 0;
	ctx->out_reading                       = DCF77_LOW_LEVEL_NO_SIGNAL;
	ctx->out_misaligned                    = 0;
	ctx->out_unidentified                  = 0;
}

void dcf77_low_level_proc(struct dcf77_low_level* ctx)
{
	if(ctx->in_val == 0) { /* no update from interrupt */

		ctx->out_misaligned = 0;

		if(++ctx->private_intervals_of_100ms_passed >= 11) {
			ctx->private_intervals_of_100ms_passed = 0;
			ctx->out_reading = DCF77_LOW_LEVEL_NO_SIGNAL;
		} else {
			ctx->out_reading = DCF77_LOW_LEVEL_NO_UPDATE;
		}

	} else { /* new value from interrupt */

		/* 1: next time query earlier */
		ctx->out_misaligned = (ctx->in_ticks_ago <= 5);

		ctx->private_intervals_of_100ms_passed = 0;

		if(ctx->in_val <= 14 && ctx->in_val >= 5) {
			ctx->out_reading = DCF77_LOW_LEVEL_0;
		} else if(ctx->in_val <= 34 && ctx->in_val >= 15) {
			ctx->out_reading = DCF77_LOW_LEVEL_1;
		} else {
			INC_SATURATED(ctx->out_unidentified);
			ctx->out_reading = DCF77_LOW_LEVEL_NO_SIGNAL;
		}
	}
}
