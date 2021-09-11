#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"

static void dcf77_timelayer_process_new_telegram(struct dcf77_timelayer* ctx,
					struct dcf77_secondlayer* secondlayer);
static void dcf77_timelayer_recover_bcd(unsigned char* telegram);
static void dcf77_timelayer_add_minute_ones_to_buffer(
			struct dcf77_timelayer* ctx, unsigned char* telegram);

void dcf77_timelayer_init(struct dcf77_timelayer* ctx)
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

void dcf77_timelayer_process(struct dcf77_timelayer* ctx,
					struct dcf77_bitlayer* bitlayer,
					struct dcf77_secondlayer* secondlayer)
{
	/* Always handle second if detected */
	if(bitlayer->out_reading != DCF77_BIT_NO_UPDATE) {
		/* TODO add 1 sec to current time! */
	}
	if(secondlayer->out_telegram_1_len != 0)
		dcf77_timelayer_process_new_telegram(ctx, secondlayer);
}

static void dcf77_timelayer_process_new_telegram(struct dcf77_timelayer* ctx,
					struct dcf77_secondlayer* secondlayer)
{
	/* 1. */
	char has_out_2 = (secondlayer->out_telegram_2_len != 0);
	dcf77_timelayer_recover_bcd(secondlayer->out_telegram_1);
	if(has_out_2)
		dcf77_timelayer_recover_bcd(secondlayer->out_telegram_2);
	dcf77_timelayer_add_minute_ones_to_buffer(ctx,
						secondlayer->out_telegram_1);
	ctx->seconds_left_in_minute = secondlayer->out_telegram_1_len;

	/* 2. TODO ... */
}

static void dcf77_timelayer_recover_bcd(unsigned char* telegram)
{
	/* TODO ... ALSO CHECK IF NOT ALREADY IMPLEMENTED SOMEWHERE ELSE? -> especially check check_bcd procedures for this. Maybe (maybe) it is even impossible to recover some things. Compare with paper notes, too! */
}

static void dcf77_timelayer_add_minute_ones_to_buffer(
			struct dcf77_timelayer* ctx, unsigned char* telegram)
{
	ctx->private_last_minute_idx = ((ctx->private_last_minute_idx + 1) %
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN);
	/* ctx->private_last_minute_ones[ctx->private_last_minute_idx] = */
	/* TODO CSTAT could use read_multiple but it would be overkill. instead hardcode how to read data! */
}
