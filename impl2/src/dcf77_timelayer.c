#include <string.h>

#include "dcf77_offsets.h"
#include "dcf77_bitlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"

static void dcf77_timelayer_process_new_telegram(struct dcf77_timelayer* ctx,
					struct dcf77_secondlayer* secondlayer);
static char dcf77_timelayer_recover_bcd(unsigned char* telegram);
static unsigned char dcf77_timelayer_read_multiple(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length);
static char dcf77_timelayer_recover_bit(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length);
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

/*
 * @return 0 if recovery incomplete and 1 if data is complete.
 */
static char dcf77_timelayer_recover_bcd(unsigned char* telegram)
{
	char rv = 1;

	/*
	 * 21-28: Minute recovery
	 *
	 * - We know that 111 (7) for minute tens is impossible.
	 *   => Recover 11X to 110.
	 *
	 *   _11X = _ 3 3 1 = 00 11 11 01 = 0x3d
	 *   _110 = _ 3 3 2 = 00 11 11 10 = 0x3e
	 */
	if(dcf77_timelayer_read_multiple(telegram, DCF77_OFFSET_MINUTE_TENS, 3)
									== 0x3d)
		dcf77_telegram_write_bit(DCF77_OFFSET_MINUTE_TENS, telegram,
								DCF77_BIT_0);
	/* - After that, recover single bit errors if exactly one remains. */
	rv &= dcf77_timelayer_recover_bit(telegram,
						DCF77_OFFSET_MINUTE_ONES, 8);

	/*
	 * 29-35: Hour recovery
	 *
	 * - We know that 11 (3) for hour tens is impossible.
	 *   => Recover 1X to 10.
	 *
	 *   __1X = _ _ 3 1 = 00 00 11 01 = 0x0d
	 *   __10 = _ _ 3 2 = 00 00 11 10 = 0x0e
	 */
	if(dcf77_timelayer_read_multiple(telegram, DCF77_OFFSET_HOUR_TENS, 2)
									== 0x0d)
		dcf77_telegram_write_bit(DCF77_OFFSET_HOUR_TENS, telegram,
								DCF77_BIT_0);
	/* - After that, recover single bit errors... */
	rv &= dcf77_timelayer_recover_bit(telegram, DCF77_OFFSET_HOUR_ONES, 7);

	/*
	 * 36-58: Date recovery
	 *
	 * - We know that DOW value 000 is invalid.
	 *   => Recover 00X to 001.
	 *
	 *   _00X = _ 2 2 1 = 00 10 10 01 = 0x29
	 *   _001 = _ 2 2 3 = 00 10 10 11 = 0x2b
	 */
	if(dcf77_timelayer_read_multiple(telegram, DCF77_OFFSET_DAY_OF_WEEK, 3)
									== 0x29)
		dcf77_telegram_write_bit(DCF77_OFFSET_DAY_OF_WEEK, telegram,
								DCF77_BIT_1);
	/*
	 * - We know that if month ones are > 2 then month tens must be = 0.
	 *   I.e. if month ones has 4-bit or higher set OR both lower bits set
	 *        then recover month ones to 0.
	 *
	 * TODO CSTAT SUBSTAT month tens recovery GOES HERE. CAN re-use from_bcd function from dcf77_secondlayer_check_bcd_correct_telgram.c. Do we store it in a header for inline inclusion or do we export it as a symbol from some source code.
	 */
	/* - After that, recover single bit errors... */
	rv &= dcf77_timelayer_recover_bit(telegram, DCF77_OFFSET_DAY_ONES, 23);
	return rv;
}

static unsigned char dcf77_timelayer_read_multiple(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length)
{
	unsigned char upper_low  = telegram[bit_offset / 4];
	unsigned char lower_up   = telegram[(bit_offset + length - 1) / 4];
	return dcf77_telegram_read_multiple_inner(upper_low, lower_up,
					bit_offset, length);
}

/*
 * Attempts to recover single bit errors by using the last bit in the offset/len
 * range as parity. If any two bits are NO_SIGNAL then recovery is impossible
 * and data is left as-is.
 *
 * @return 0 if at least one of the data bits (all in range offset..len-2) is
 * 	undefined. 1 if all data bits are defined.
 */
static char dcf77_timelayer_recover_bit(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length)
{
	/* TODO N_IMPL */
	return 0;
}

static void dcf77_timelayer_add_minute_ones_to_buffer(
			struct dcf77_timelayer* ctx, unsigned char* telegram)
{
	ctx->private_last_minute_idx = ((ctx->private_last_minute_idx + 1) %
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN);
	/* ctx->private_last_minute_ones[ctx->private_last_minute_idx] = */
	/* TODO CSTAT could use read_multiple but it would be overkill. instead hardcode how to read data! */
}