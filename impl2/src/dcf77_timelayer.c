#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"

void dcf77_timelayer_init(struct dcf77_timelayer_ctx* ctx)
{
	const struct dcf77_timelayer_tm tm0 = DCF77_TIMELAYER_T_COMPILATION;
	ctx->private_last_minute_idx = DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN - 1;
	ctx->private_num_seconds_since_prev = DCF77_TIMELAYER_PREV_UNKNOWN;
	ctx->seconds_left_in_minute = 60;
	ctx->out_current = tm0;
	memset(ctx->private_last_minute_ones, 0, sizeof(unsigned char) *
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN);
	memset(ctx->private_prev_telegram, 0, sizeof(unsigned char) *
					DCF77_SECONDLAYER_LINE_BYTES);
}

void dcf77_timelayer_process(struct dcf77_timelayer_ctx* ctx,
					struct dcf77_bitlayer* bitlayer,
					struct dcf77_secondlayer* secondlayer)
{
	/* TODO CSTAT N_IMPL / CONSULT PAPER NOTES */
}
