#include <string.h>

#include "dcf77_offsets.h"
#include "dcf77_bitlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"
#include "dcf77_bcd.h"

static const unsigned char MONTH_LENGTHS[] = {
	29, /*  0: leap year February */
	31, /*  1: January            */
	28, /*  2: February           */
	31, /*  3: March              */
	30, /*  4: April              */
	31, /*  5: May                */
	30, /*  6: June               */
	31, /*  7: July               */
	31, /*  8: August             */
	30, /*  9: September          */
	31, /* 10: October            */
	30, /* 11: November           */
	31, /* 12: December           */
};

static void dcf77_timelayer_advance_tm_by_sec(struct dcf77_timelayer_tm* tm,
								short seconds);
static char dcf77_timelayer_is_leap_year(short y);
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
		/*
		 * Add one sec to current time handling leap seconds and prev
		 * management.
		 *
		 * TODO CSTAT SMALL PROBLEM: HOW IS A CASE HANDLED WHERE WE ARE IN PERFECT SYNC. BY CURRENT IMPLEMENTATION WE WOULD SET PREV HERE TO A "COMPUTED" VALUE ONLY TO UPDATE IT FURTHER DOWNWARDS TO THE ACTUAL VALUE? NEED TO SOMEHOW INVERT THE HANDLING HERE S.T. SYNCED BEHAVIOUR OVERRIDES/PRECEDES THE GENERIC +1 SEC COMPUTATION!
		 */
		if(ctx->private_num_seconds_since_prev < 0x7fff)
			ctx->private_num_seconds_since_prev++;

		if(ctx->seconds_left_in_minute > 0)
			ctx->seconds_left_in_minute--;
		if(ctx->seconds_left_in_minute == 1 &&
						ctx->out_current.s == 59) {
			/* special leap second case */
			ctx->out_current.s = 60;
		} else {
			if(ctx->out_current.s >= 59) {
				/*
				 * We now have to store the out_current as prev.
				 */
				ctx->private_prev = ctx->out_current;
				ctx->private_num_seconds_since_prev = 0;
				/* TODO x UPDATE PRIVATE PREV TELEGRAM */
			}
			dcf77_timelayer_advance_tm_by_sec(&ctx->out_current, 1);
		}
	}
	if(secondlayer->out_telegram_1_len != 0)
		dcf77_timelayer_process_new_telegram(ctx, secondlayer);
}

/* Not leap-second aware for now */
static void dcf77_timelayer_advance_tm_by_sec(struct dcf77_timelayer_tm* tm,
								short seconds)
{
	unsigned char midx;

	tm->s += seconds;
	if(tm->s >= 60) {
		tm->i += tm->s / 60;
		tm->s  = tm->s % 60;

		if(tm->i >= 60) {
			tm->h += tm->i / 60;
			tm->i  = tm->i % 60;

			if(tm->h >= 24) {
				tm->d += tm->h / 24;
				tm->h  = tm->h % 24;

				midx = dcf77_timelayer_is_leap_year(tm->y)?
								0: tm->m;
				if(tm->d > MONTH_LENGTHS[midx]) {
					tm->d -= MONTH_LENGTHS[midx];
					tm->m++;

					if(tm->m > 12) {
						tm->m = 1;
						tm->y++;
					}
				}
			}
		}
	}
}

/* https://en.wikipedia.org/wiki/Leap_year */
static char dcf77_timelayer_is_leap_year(short y)
{
	/* TODO z MAKE AN EXECUTABLE TEST FOR THIS! */
	/* Test cases: 2020, 2024, 2028 -> 1 */
	/*             2000             -> 1 */
	/*             1900             -> 0 */
	return (y % 4) == 0 && (((y % 100) != 0) || (y % 400 == 0));
}

static void dcf77_timelayer_process_new_telegram(struct dcf77_timelayer* ctx,
					struct dcf77_secondlayer* secondlayer)
{
	/* char xeliminate_prev; * 2a. */

	/* 1. */
	char has_out_2 = (secondlayer->out_telegram_2_len != 0);
	char out_1_is_complete =
		dcf77_timelayer_recover_bcd(secondlayer->out_telegram_1);
	if(has_out_2)
		dcf77_timelayer_recover_bcd(secondlayer->out_telegram_2);
	dcf77_timelayer_add_minute_ones_to_buffer(ctx,
						secondlayer->out_telegram_1);
	ctx->seconds_left_in_minute = secondlayer->out_telegram_1_len;

	/* 2. */
	if(out_1_is_complete) {
		/* TODO set current time = decode(secondlayer->out_telegram_1) and adjust prev structure, return; -> QOS1 TODO CSTAT EITHER THIS OR ADD 1 SEC DATETIME MODEL FUNCTION NEEDS TO BE IMPLEMENTED. */
	}

	/* 3. */
	/* TODO if recover ones() ... */
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
	 *        then recover month tens to 0.
	 */
	if(dcf77_telegram_read_bit(DCF77_OFFSET_MONTH_TENS, telegram) ==
							DCF77_BIT_NO_SIGNAL &&
			dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
					DCF77_OFFSET_MONTH_ONES, 4)) > 2)
		dcf77_telegram_write_bit(DCF77_OFFSET_MONTH_TENS, telegram,
								DCF77_BIT_0);
	/* - After that, recover single bit errors... */
	rv &= dcf77_timelayer_recover_bit(telegram, DCF77_OFFSET_DAY_ONES, 23);
	return rv;
}

static unsigned char dcf77_timelayer_read_multiple(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length)
{
	unsigned char upper_low = telegram[bit_offset / 4];
	unsigned char lower_up  = telegram[(bit_offset + length - 1) / 4];
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
	char known_missing = -1;
	unsigned char i;
	unsigned char last_excl = bit_offset + length;
	unsigned char parity = 0; /* counts number of ones in sequence */

	for(i = bit_offset; i < last_excl; i++) {
		switch(dcf77_telegram_read_bit(i, telegram)) {
		case DCF77_BIT_NO_SIGNAL:
			if(known_missing == -1)
				/* It's the first no signal. Store position. */
				known_missing = (char)i;
			else
				/* More than one bit missing. Cannot recover. */
				return 0;
			break;
		case DCF77_BIT_1:
			parity++;
			break;
		default:
			/*
			 * pass, do not update parity
			 *
			 * Having NO_UPDATE here would be an error but it is not
			 * handled here!
			 */
			break;
		}
	}
	if((parity % 2) == 0) {
		/*
		 * Even parity is accepted hence recover potential missing bit
		 * to 0.
		 */
		if(known_missing != -1)
			dcf77_telegram_write_bit(known_missing, telegram,
								DCF77_BIT_0);
		/* In any case this means parity passed! */
		return 1;
	} else if(known_missing == -1) {
		/*
		 * Odd number of ones means there must be missing something.
		 * However, if we do not have an index for this it means the
		 * data is bad. This is an error case!
		 *
		 * Say "recovery failed" to indicate that something is fishy
		 * and the data better not be trusted from the segment of
		 * interest.
		 */
		return 0;
	} else {
		/*
		 * Odd number of ones and missing index exists means we recover
		 * the missing bit to 1.
		 */
		dcf77_telegram_write_bit(known_missing, telegram, DCF77_BIT_1);
		return 1;
	}
}

static void dcf77_timelayer_add_minute_ones_to_buffer(
			struct dcf77_timelayer* ctx, unsigned char* telegram)
{
	ctx->private_last_minute_idx = ((ctx->private_last_minute_idx + 1) %
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN);
	ctx->private_last_minute_ones[ctx->private_last_minute_idx] =
					dcf77_timelayer_read_multiple(telegram,
					DCF77_OFFSET_MINUTE_ONES, 4);
}
