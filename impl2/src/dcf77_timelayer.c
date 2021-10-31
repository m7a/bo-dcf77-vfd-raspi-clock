#include <string.h>

#include "dcf77_offsets.h"
#include "dcf77_bitlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_secondlayer.h"
#include "dcf77_secondlayer_xeliminate.h"
#include "dcf77_timelayer.h"
#include "dcf77_bcd.h"

enum dcf77_timelayer_recovery {
	DCF77_TIMELAYER_DATA_IS_COMPLETE = 1,
	DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MINUTE = 2,
	DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MULTIPLE = 3,
};

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

static const struct dcf77_timelayer_tm TM0 = DCF77_TIMELAYER_T_COMPILATION;

static const unsigned char DCF77_TIMELAYER_BCD_COMPARISON_SEQUENCE[] = {
	/* Decimal - Binary  - Internal    -    Hex  */
	/* 0       - 0 0 0 0 - 10 10 10 10 - */ 0xaa,
	/* 1       - 0 0 0 1 - 10 10 10 11 - */ 0xab,
	/* 2       - 0 0 1 0 - 10 10 11 10 - */ 0xae,
	/* 3       - 0 0 1 1 - 10 10 11 11 - */ 0xaf,
	/* 4       - 0 1 0 0 - 10 11 10 10 - */ 0xba,
	/* 5       - 0 1 0 1 - 10 11 10 11 - */ 0xbb,
	/* 6       - 0 1 1 0 - 10 11 11 10 - */ 0xbe,
	/* 7       - 0 1 1 1 - 10 11 11 11 - */ 0xbf,
	/* 8       - 1 0 0 0 - 11 10 10 10 - */ 0xea,
	/* 9       - 1 0 0 1 - 11 10 10 11 - */ 0xeb,
};
static const unsigned char DCF77_TIMELAYER_BCD_CMP_LEN =
	sizeof(DCF77_TIMELAYER_BCD_COMPARISON_SEQUENCE) / sizeof(unsigned char);

#ifdef TEST
#define EXPORTED_FOR_TESTING
#else
#define EXPORTED_FOR_TESTING static
static char dcf77_timelayer_are_ones_compatible(unsigned char ones0,
							unsigned char ones1);
static char dcf77_timelayer_is_leap_year(short y);
static void dcf77_timelayer_advance_tm_by_sec(struct dcf77_timelayer_tm* tm,
								short seconds);
static char dcf77_timelayer_recover_ones(struct dcf77_timelayer* ctx);
#endif

static void dcf77_timelayer_tm_to_telegram_10min(struct dcf77_timelayer_tm* tm,
						unsigned char* out_telegram);
static void dcf77_timelayer_write_multiple_bits_converting(
			unsigned char* out_telegram, unsigned char from_bit,
			unsigned char num_bits, unsigned char in_bits);
static void dcf77_timelayer_process_new_telegram(struct dcf77_timelayer* ctx,
					struct dcf77_secondlayer* secondlayer);
static enum dcf77_timelayer_recovery dcf77_timelayer_recover_bcd(
						unsigned char* telegram);
static unsigned char dcf77_timelayer_read_multiple(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length);
static char dcf77_timelayer_recover_bit(unsigned char* telegram,
				unsigned char bit_offset, unsigned char length);
static void dcf77_timelayer_decode_and_populate_dst_switch(
			struct dcf77_timelayer* ctx, unsigned char* telegram);
static void dcf77_timelayer_add_minute_ones_to_buffer(
			struct dcf77_timelayer* ctx, unsigned char* telegram);
static char dcf77_timelayer_decode_check(struct dcf77_timelayer_tm* tm,
						unsigned char* telegram);
static void dcf77_timelayer_decode(struct dcf77_timelayer_tm* tm,
						unsigned char* telegram);
static char dcf77_timelayer_has_minute_tens(unsigned char* telegram);
static void dcf77_timelayer_decode_tens(struct dcf77_timelayer_tm* tm,
						unsigned char* telegram);
static char dcf77_timelayer_tm_eq(struct dcf77_timelayer_tm* a,
						struct dcf77_timelayer_tm* b);
static char dcf77_timelayer_check_if_current_compat_by_xeliminate(
	struct dcf77_timelayer* ctx, struct dcf77_secondlayer* secondlayer);
static void dcf77_timelayer_tm_to_telegram(struct dcf77_timelayer_tm* tm,
						unsigned char* out_telegram);
static char dcf77_timelayer_cross_check_by_xeliminate(
	struct dcf77_timelayer* ctx, struct dcf77_secondlayer* secondlayer);

void dcf77_timelayer_init(struct dcf77_timelayer* ctx)
{
	ctx->private_preceding_minute_idx =
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN - 1;
	ctx->private_num_seconds_since_prev = DCF77_TIMELAYER_PREV_UNKNOWN;
	ctx->private_seconds_left_in_minute = 60;
	ctx->out_current = TM0;
	ctx->out_qos = DCF77_TIMELAYER_QOS9_ASYNC;
	memset(ctx->private_preceding_minute_ones, 0, sizeof(unsigned char) *
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
		 * Two options:
		 * (a) double compute i.e. first +1sec then see if telegram
		 *     processing is compatible if not update.
		 * (b) only compute +1sec if we are within a minute. if we are
		 *     at the beginning of a new minute then _first_ try to
		 *     process telegram and only revert to +1 routine if that
		 *     does not successfully yield the current time to display.
		 *
		 * CURSEL: (a) double compute to allow setting prev minute from
		 *     the model for cases where secondlayer does not provide
		 *     us with one. This allows using the model without the
		 *     necessity to generate previous date values.
		 */
		if(ctx->private_num_seconds_since_prev < 0x7fff)
			ctx->private_num_seconds_since_prev++;

		if(ctx->private_seconds_left_in_minute > 0)
			ctx->private_seconds_left_in_minute--;
		if(ctx->private_seconds_left_in_minute == 1 &&
						ctx->out_current.s == 59) {
			/* special leap second case */
			ctx->out_current.s = 60;
		} else {
			/*
			 * Store out_current as prev if +1 yields a new minute
			 * and our current minute is X9 i.e. next minute will
			 * be Y0 with Y = X+1 (or more fields changed...)
			 */
			if(ctx->out_current.s >= 59) {
				/*
				 * If this is the only opinion we get here, it
				 * essentially means that we are not in sync!
				 */
				ctx->out_qos = DCF77_TIMELAYER_QOS9_ASYNC;

				if(ctx->out_current.i == 59 &&
						ctx->private_eoh_dst_switch !=
						DCF77_TIMELAYER_DST_NO_CHANGE) {
					/* TODO z TOO MANY LAYERS OF INDENTATION. EXTERNALIZE FUNCTION. */
					/* TODO z MINOR HACK: ONLY SUPPORTS LEAP IF NOT ACROSS DAYS I.E. HOUR CHANGES WITHIN DAY. NO CHECKS ARE PREFORMED! THIS IS THE EXPECTED CONVENTION IN DE WHERE WE ONLY EVER SWITCH BETWEEN 02:00 (01:59) and 03:00 (02:59) times! */
					switch(ctx->private_eoh_dst_switch) {
					case DCF77_TIMELAYER_DST_TO_SUMMER:
						/* summer = UTC+2, +1h */
						ctx->out_current.h++;
						/* dcf77_timelayer_advance_tm_by_sec(&ctx->out_current, 3600); */
						break;
					case DCF77_TIMELAYER_DST_TO_WINTER:
						/* winter = UTC+1, -1h */
						ctx->out_current.h--;
						/* dcf77_timelayer_advance_tm_by_sec(&ctx->out_current, -3600); */
						break;
					default:
						break; /* ignore */
					}
					/*
					 * set prev to unknown because now we
					 * move more than a mere ten minute
					 * step.
					 */
					ctx->private_num_seconds_since_prev = DCF77_TIMELAYER_PREV_UNKNOWN;
					ctx->private_eoh_dst_switch = DCF77_TIMELAYER_DST_SWITCH_APPLIED;
				} else if((ctx->out_current.i % 10) == 9) {
					ctx->private_prev = ctx->out_current;
					ctx->private_num_seconds_since_prev = 0;
					dcf77_timelayer_tm_to_telegram_10min(
						&ctx->private_prev,
						ctx->private_prev_telegram);
				}
			}
			dcf77_timelayer_advance_tm_by_sec(&ctx->out_current, 1);
		}
	}
	if(secondlayer->out_telegram_1_len != 0) {
		dcf77_timelayer_process_new_telegram(ctx, secondlayer);
		if(ctx->private_eoh_dst_switch ==
					DCF77_TIMELAYER_DST_SWITCH_APPLIED)
			ctx->private_eoh_dst_switch =
					DCF77_TIMELAYER_DST_NO_CHANGE;
	}
}

/*
 * Currently needed to store "prev" values if +1sec yields a new minute tens.
 * Currently only stores tm to ten minute precision.
 */
static void dcf77_timelayer_tm_to_telegram_10min(struct dcf77_timelayer_tm* tm,
						unsigned char* out_telegram)
{
	unsigned char y_val = tm->y % 100;
	memset(out_telegram, 0x55, sizeof(unsigned char) *
						DCF77_SECONDLAYER_LINE_BYTES);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_YEAR_ONES, DCF77_LENGTH_YEAR_ONES, y_val % 10);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_YEAR_TENS, DCF77_LENGTH_YEAR_TENS, y_val / 10);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_DAY_ONES, DCF77_LENGTH_DAY_ONES, tm->d % 10);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_DAY_TENS, DCF77_LENGTH_DAY_TENS, tm->d / 10);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_HOUR_ONES, DCF77_LENGTH_HOUR_ONES, tm->h % 10);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_HOUR_TENS, DCF77_LENGTH_HOUR_TENS, tm->h / 10);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_MINUTE_TENS, DCF77_LENGTH_MINUTE_TENS, tm->i / 10);
}

/* TODO z PERFORMANCE OF THIS ROUTINE MIGHT BE OPTIMIZABLE... */
static void dcf77_timelayer_write_multiple_bits_converting(
			unsigned char* out_telegram, unsigned char from_bit,
			unsigned char num_bits, unsigned char in_bits)
{
	/* write lowest bit first */
	unsigned char bits_processed;
	unsigned char val_to_write;
	for(bits_processed = 0; bits_processed < num_bits; bits_processed++) {
		val_to_write = (in_bits & 1)? DCF77_BIT_1: DCF77_BIT_0;
		dcf77_telegram_write_bit(from_bit + bits_processed,
						out_telegram, val_to_write);
	}
}

/*
 * Not leap-second aware for now
 * assert seconds < 12000, otherwise may output incorrect results!
 * TODO NEEDS TO WORK WITH seconds = -3600, -3601 too!
 */
EXPORTED_FOR_TESTING void dcf77_timelayer_advance_tm_by_sec(
				struct dcf77_timelayer_tm* tm, short seconds)
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

				/*
				 * In case of leap year, access index 0 to
				 * return length of 29 days for Feburary in
				 * leap years.
				 */
				midx = (tm->m == 2 &&
					dcf77_timelayer_is_leap_year(tm->y))?
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
EXPORTED_FOR_TESTING char dcf77_timelayer_is_leap_year(short y)
{
	return (y % 4) == 0 && (((y % 100) != 0) || (y % 400 == 0));
}

static void dcf77_timelayer_process_new_telegram(struct dcf77_timelayer* ctx,
					struct dcf77_secondlayer* secondlayer)
{
	/* char xeliminate_prev; * 2a. */
	char recovered_ones;
	struct dcf77_timelayer_tm intermediate;
	/*
	 * Initialize to "true" value in order to later use this to detect
	 * that xeliminate was not performed yet.
	 */
	char xeliminate_prev = 2;

	/* 1. */
	char has_out_2 = (secondlayer->out_telegram_2_len != 0);
	enum dcf77_timelayer_recovery out_1_recovery =
				dcf77_timelayer_recover_bcd(
				secondlayer->out_telegram_1);
	enum dcf77_timelayer_recovery out_2_recovery = has_out_2?
				dcf77_timelayer_recover_bcd(
				secondlayer->out_telegram_2):
				DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MULTIPLE;
	char has_out_2_tens =
		(out_2_recovery == DCF77_TIMELAYER_DATA_IS_COMPLETE ||
		(out_2_recovery ==
			DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MINUTE &&
		dcf77_timelayer_has_minute_tens(secondlayer->out_telegram_2)));
	dcf77_timelayer_add_minute_ones_to_buffer(ctx,
						secondlayer->out_telegram_1);
	ctx->private_seconds_left_in_minute = secondlayer->out_telegram_1_len;

	dcf77_timelayer_decode_and_populate_dst_switch(ctx,
						secondlayer->out_telegram_1);

	/* 1a. pre-adjust previous in case we can safely decode it */
	if(has_out_2_tens) {
		dcf77_timelayer_decode_tens(&ctx->private_prev,
						secondlayer->out_telegram_2);
		memcpy(ctx->private_prev_telegram, secondlayer->out_telegram_2,
			DCF77_SECONDLAYER_LINE_BYTES * sizeof(unsigned char));
		ctx->private_num_seconds_since_prev = 0;
	}

	/* 2. */
	if(out_1_recovery == DCF77_TIMELAYER_DATA_IS_COMPLETE) {
		if(!dcf77_timelayer_decode_check(&ctx->out_current,
						secondlayer->out_telegram_1) &&
				out_2_recovery ==
				DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MULTIPLE)
			/* computed previous can not be accepted */
			ctx->private_num_seconds_since_prev =
						DCF77_TIMELAYER_PREV_UNKNOWN;
		ctx->out_qos = DCF77_TIMELAYER_QOS1;
		return;
	}

	/* 3. */
	if((recovered_ones = dcf77_timelayer_recover_ones(ctx)) != -1) {
		/*
		 * 3.1 If has ymdhi tens (current),
		 *     out1 recovery == complete handled above already
		 */
		if(out_1_recovery ==
		DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MINUTE &&
		dcf77_timelayer_has_minute_tens(secondlayer->out_telegram_1)) {
			dcf77_timelayer_decode_tens(&intermediate,
						secondlayer->out_telegram_1);
			dcf77_timelayer_advance_tm_by_sec(&intermediate,
						recovered_ones * 60);
			if(!dcf77_timelayer_tm_eq(&intermediate,
							&ctx->out_current)) {
				ctx->out_current = intermediate;
				if(out_2_recovery ==
				DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MULTIPLE)
					/* discard potentially invalid prev */
					ctx->private_num_seconds_since_prev =
						DCF77_TIMELAYER_PREV_UNKNOWN;
			} /* else is equal and prev remains valid! */
			ctx->out_qos = DCF77_TIMELAYER_QOS2;
			return;
		}
		/* 3.2 if has ymdhi tens (prev) */
		if(has_out_2_tens) {
			/* we know that they are already decoded above */
			ctx->out_current = ctx->private_prev;
			/* prev + 10min + recovered_ones */
			dcf77_timelayer_advance_tm_by_sec(&ctx->out_current,
						recovered_ones * 60 + 600);
			ctx->private_num_seconds_since_prev =
							recovered_ones * 60;
			ctx->out_qos = DCF77_TIMELAYER_QOS3;
			return;
		}
		/*
		 * 3.3 xeliminate_prev = xeliminate(secondlayer.prev,prev)
		 *
		 * NB: Writes to private_prev_telegram in order to make it
		 *     as precise as possible. Need to consider this when
		 *     handling subsequent QOS5 case.
		 *
		 * Extra parens to silence compiler warning.
		 */
		if((xeliminate_prev = dcf77_secondlayer_xeliminate(0, 
					secondlayer->out_telegram_2,
					ctx->private_prev_telegram))) {
			ctx->out_current = ctx->private_prev;
			ctx->out_current.i = 0; /* use prev tens ignore ones */
			/* current = prev tens + 10min + recoveredOnes */
			dcf77_timelayer_advance_tm_by_sec(&ctx->out_current,
						recovered_ones * 60 + 600);
			ctx->out_qos = DCF77_TIMELAYER_QOS4;
			/*
			 * NB: In case xeliminate_prev = 1 i.e. successful,
			 * control flow returns here. From beyond this return
			 * onwards, xeliminate_prev is 0 iff 3.3 was reached but
			 * its condition failed.
			 */
			return;
		}
	}

	/* 4. */
	if(dcf77_timelayer_check_if_current_compat_by_xeliminate(ctx,
								secondlayer)) {
		/*
		 * QOS5: The automatically computed time is compatible with
		 * the received (possibly incomplete) telegram. Hence we now
		 * run on our own clock but know that the data is at least not
		 * contrary to the signals received despite the fact that we
		 * currently cannot decode them properly.
		 */
		ctx->out_qos = DCF77_TIMELAYER_QOS5;
		return;
	}

	/* 5. */
	if(ctx->private_num_seconds_since_prev
					!= DCF77_TIMELAYER_PREV_UNKNOWN) {
		if(xeliminate_prev == 2) {
			/*
			 * Means we did not do xeliminate for prev yet,
			 * do it now.
			 */
			xeliminate_prev = dcf77_secondlayer_xeliminate(0, 
						secondlayer->out_telegram_2,
						ctx->private_prev_telegram);
		}
		if(xeliminate_prev) {
			ctx->out_current = ctx->private_prev;
			ctx->out_current.i = 0; /* use prev tens ignore ones */
			/* current = prev tens + 10min + num seconds since */
			dcf77_timelayer_advance_tm_by_sec(&ctx->out_current,
				600 + ctx->private_num_seconds_since_prev);
			ctx->private_num_seconds_since_prev += 600; /* TODO z CORRECT? */
			ctx->out_qos = DCF77_TIMELAYER_QOS6;
			return;
		}
		/*
		 * 5ABC: Spot misalignments. If xeliminate_prev fails, then do
		 * some cross checks
		 */
		if(dcf77_timelayer_cross_check_by_xeliminate(ctx,
								secondlayer)) {
			/*
			 * Time adjustments performed in
			 * cross_check_by_xeliminate
			 */
			return;
		}
	}

	/* Nothing more to try, now runs async. */
	ctx->out_qos = DCF77_TIMELAYER_QOS9_ASYNC;
}

static enum dcf77_timelayer_recovery dcf77_timelayer_recover_bcd(
							unsigned char* telegram)
{
	char rv = DCF77_TIMELAYER_DATA_IS_COMPLETE;

	/*
	 * 21-28: Minute recovery
	 *
	 * - We know that 111 (7) for minute tens is impossible.
	 *   => Recover 11X to 110.
	 *
	 *   _11X = _ 3 3 1 = 00 11 11 01 = 0x3d
	 *   _110 = _ 3 3 2 = 00 11 11 10 = 0x3e
	 */
	if(dcf77_timelayer_read_multiple(telegram, DCF77_OFFSET_MINUTE_TENS,
					DCF77_LENGTH_MINUTE_TENS) == 0x3d)
		dcf77_telegram_write_bit(DCF77_OFFSET_MINUTE_TENS, telegram,
								DCF77_BIT_0);
	/* - After that, recover single bit errors if exactly one remains. */
	if(!dcf77_timelayer_recover_bit(telegram, DCF77_OFFSET_MINUTE_ONES, 8))
		rv = DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MINUTE;

	/*
	 * 29-35: Hour recovery
	 *
	 * - We know that 11 (3) for hour tens is impossible.
	 *   => Recover 1X to 10.
	 *
	 *   __1X = _ _ 3 1 = 00 00 11 01 = 0x0d
	 *   __10 = _ _ 3 2 = 00 00 11 10 = 0x0e
	 */
	if(dcf77_timelayer_read_multiple(telegram, DCF77_OFFSET_HOUR_TENS,
						DCF77_LENGTH_HOUR_TENS) == 0x0d)
		dcf77_telegram_write_bit(DCF77_OFFSET_HOUR_TENS, telegram,
								DCF77_BIT_0);
	/* - After that, recover single bit errors... */
	if(!dcf77_timelayer_recover_bit(telegram, DCF77_OFFSET_HOUR_ONES, 7))
		rv = DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MULTIPLE;

	/*
	 * 36-58: Date recovery
	 *
	 * - We know that DOW value 000 is invalid.
	 *   => Recover 00X to 001.
	 *
	 *   _00X = _ 2 2 1 = 00 10 10 01 = 0x29
	 *   _001 = _ 2 2 3 = 00 10 10 11 = 0x2b
	 */
	if(dcf77_timelayer_read_multiple(telegram, DCF77_OFFSET_DAY_OF_WEEK,
					DCF77_LENGTH_DAY_OF_WEEK) == 0x29)
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
						DCF77_OFFSET_MONTH_ONES,
						DCF77_LENGTH_MONTH_ONES)) > 2)
		dcf77_telegram_write_bit(DCF77_OFFSET_MONTH_TENS, telegram,
								DCF77_BIT_0);
	/* - After that, recover single bit errors... */
	if(!dcf77_timelayer_recover_bit(telegram, DCF77_OFFSET_DAY_ONES, 23))
		rv = DCF77_TIMELAYER_DATA_IS_INCOMPLETE_FOR_MULTIPLE;

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

static void dcf77_timelayer_decode_and_populate_dst_switch(
			struct dcf77_timelayer* ctx, unsigned char* telegram)
{
	
	unsigned char announce = dcf77_telegram_read_bit(
					DCF77_OFFSET_DST_ANNOUNCE, telegram);
	/*
	 * It is either 10 = 11 10 = 0x0e = currently summer time
	 *       xor    01 = 10 11 = 0x0b = currently winter time
	 */
	unsigned char current_tz = dcf77_timelayer_read_multiple(telegram,
					DCF77_OFFSET_DAYLIGHT_SAVING_TIME,
					DCF77_LENGTH_DAYLIGHT_SAVING_TIME);

	if(announce == DCF77_BIT_0) {
		ctx->private_eoh_dst_switch = DCF77_TIMELAYER_DST_NO_CHANGE;
	} else if(announce == DCF77_BIT_1 && ctx->private_eoh_dst_switch ==
					DCF77_TIMELAYER_DST_SWITCH_APPLIED) {
		/*
		 * if we have just applied the switch, then do not propose
		 * it for the next minute.
		 */
		if(current_tz == 0x0e) {
			ctx->private_eoh_dst_switch =
					DCF77_TIMELAYER_DST_TO_WINTER;
		} else if(current_tz == 0x0f) {
			ctx->private_eoh_dst_switch =
					DCF77_TIMELAYER_DST_TO_SUMMER;
		} /* else ignore if not specified exactly */
	} /* else ignore if no data available */
}

static void dcf77_timelayer_add_minute_ones_to_buffer(
			struct dcf77_timelayer* ctx, unsigned char* telegram)
{
	ctx->private_preceding_minute_idx = ((ctx->private_preceding_minute_idx
			+ 1) % DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN);
	ctx->private_preceding_minute_ones[ctx->private_preceding_minute_idx] =
			dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_MINUTE_ONES, DCF77_LENGTH_MINUTE_ONES);
}

/* @return 1 if no differences were detected */
static char dcf77_timelayer_decode_check(struct dcf77_timelayer_tm* tm,
							unsigned char* telegram)
{
	unsigned char rv;
	struct dcf77_timelayer_tm tmn; /* intermediate new time stamp */
	dcf77_timelayer_decode(&tmn, telegram);
	rv = (memcmp(&tmn, tm, sizeof(struct dcf77_timelayer_tm)) == 0);
	*tm = tmn;
	return rv;
}

static void dcf77_timelayer_decode(struct dcf77_timelayer_tm* tm,
							unsigned char* telegram)
{
	tm->y = (TM0.y / 100) * 100
		+ dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_YEAR_TENS, DCF77_LENGTH_YEAR_TENS)) * 10
		+ dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_YEAR_ONES, DCF77_LENGTH_YEAR_ONES));
	tm->m = dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_MONTH_TENS, DCF77_LENGTH_MONTH_TENS)) * 10
		+ dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_MONTH_ONES, DCF77_LENGTH_MONTH_ONES));
	tm->d = dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_DAY_TENS, DCF77_LENGTH_DAY_TENS)) * 10
		+ dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_DAY_ONES, DCF77_LENGTH_DAY_ONES));
	tm->h = dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_HOUR_TENS, DCF77_LENGTH_HOUR_TENS)) * 10
		+ dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_HOUR_ONES, DCF77_LENGTH_HOUR_ONES));
	tm->i = dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_MINUTE_TENS, DCF77_LENGTH_MINUTE_TENS))*10
		+ dcf77_bcd_from(dcf77_timelayer_read_multiple(telegram,
			DCF77_OFFSET_MINUTE_ONES, DCF77_LENGTH_MINUTE_ONES));
	tm->s = 0;
}

/* @return value if ones were recovered successfully, -1 if not. */
/* TODO RC: TEST THIS PROCEDURE INDIVIDUALLY / TEST PREPARED BUT NEED SOME CASE WITH DISTORTION! */
EXPORTED_FOR_TESTING char dcf77_timelayer_recover_ones(
						struct dcf77_timelayer* ctx)
{
	unsigned char idx_compare;
	unsigned char idx_delta;
	unsigned char idx_preceding = ctx->private_preceding_minute_idx;

	/* TODO z not memory efficient, could use bit fiddling if needed */
	unsigned char idx_pass[DCF77_TIMELAYER_BCD_CMP_LEN];

	char found_idx = -1;

	memset(idx_pass, 1, sizeof(idx_pass) / sizeof(unsigned char));

	for(idx_compare = 0; idx_compare < DCF77_TIMELAYER_BCD_CMP_LEN;
								idx_compare++) {
		idx_delta = idx_compare;
		do {
			if(!dcf77_timelayer_are_ones_compatible(
			DCF77_TIMELAYER_BCD_COMPARISON_SEQUENCE[idx_delta],
			ctx->private_preceding_minute_ones[idx_preceding])) {
				idx_pass[idx_compare] = 0;
				idx_preceding =
					ctx->private_preceding_minute_idx;
				break;
			}
			idx_preceding = (idx_preceding + 1) %
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN;
			idx_delta = (idx_delta + 1) %
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN;
		} while(idx_preceding != ctx->private_preceding_minute_idx);
	}

	for(idx_compare = 0; idx_compare < DCF77_TIMELAYER_BCD_CMP_LEN;
								idx_compare++) {
		if(idx_pass[idx_compare]) {
			if(found_idx == -1)
				found_idx = idx_compare;
			else
				return -1; /* another match => not unique */
		}
	}

	return found_idx;
}

/*
 * Like xeliminate but without changing the values:
 * Returns 1 if ones0 and ones1 could denote the same number.
 * Returns 0 if they must represent different numbers.
 */
EXPORTED_FOR_TESTING char dcf77_timelayer_are_ones_compatible(
				unsigned char ones0, unsigned char ones1)
{
	/*
	 * Now we make use of the specific encoding
	 * A leading 0 means the value is an epsilon/no signal.
	 * A leading 1 means the value is a detected signal value.
	 *
	 * ones0 & ones1 has leading "1" set if both values were detected values
	 *               by doing & 0b10101010 (0xaa) we get only the ones
	 *               which indicate "dected value set".
	 *               In case all values weere detected this would be
	 *               10101010. Now shift one rightwards to 01010101 to
	 *               create a pattern that can be compared against the next
	 *               step.
	 *
	 * ones0 ^ ones1 has trailing "1" if both values differed there.
	 *               by ding & 0b01010101 (0x55) we extract only the
	 *               value-relevant parts.
	 *
	 * Now by &-ing together the first and the second ones we get a value
	 * that is 0 if differences only occurred in places where one of the
	 * values was not detected i.e. 0 means they are compatible. Do a
	 * logical inversion to return 1 if compatible.
	 */
	return !((((ones0 & ones1) & 0xaa) >> 1) & ((ones0 ^ ones1) & 0x55));
}

static char dcf77_timelayer_has_minute_tens(unsigned char* telegram)
{
	/* if all relevant bits have leading `1` then it means OK */
	return (dcf77_timelayer_read_multiple(telegram,
		DCF77_OFFSET_MINUTE_TENS, DCF77_LENGTH_MINUTE_TENS) & 0x2a)
		== 0x2a;
}

static void dcf77_timelayer_decode_tens(struct dcf77_timelayer_tm* tm,
							unsigned char* telegram)
{
	dcf77_timelayer_decode(tm, telegram);
	tm->i = (tm -> i / 10) * 10; /* discard unwanted ones */
}

static char dcf77_timelayer_tm_eq(struct dcf77_timelayer_tm* a,
					struct dcf77_timelayer_tm* b)
{
	return memcmp(a, b, sizeof(struct dcf77_timelayer_tm)) == 0;
}

static char dcf77_timelayer_check_if_current_compat_by_xeliminate(
	struct dcf77_timelayer* ctx, struct dcf77_secondlayer* secondlayer)
{
	unsigned char virtual_telegram[DCF77_SECONDLAYER_LINE_BYTES];
	dcf77_timelayer_tm_to_telegram(&ctx->out_current, virtual_telegram);
	return dcf77_secondlayer_xeliminate(0, secondlayer->out_telegram_2,
							virtual_telegram);
}

/* Variant with one minute precision! */
static void dcf77_timelayer_tm_to_telegram(struct dcf77_timelayer_tm* tm,
						unsigned char* out_telegram)
{
	dcf77_timelayer_tm_to_telegram_10min(tm, out_telegram);
	dcf77_timelayer_write_multiple_bits_converting(out_telegram,
		DCF77_OFFSET_MINUTE_ONES, DCF77_LENGTH_MINUTE_ONES, tm->i % 10);
}

/*
 * 5A: xeliminate(secondlayer current, ctx prev) if matches
 *     then let time = ctx prev.
 * 5B: xeliminate(secondlayer current, ctx current + 1min) if matches
 *     then let time = ctx current + 1min
 * 5C: if both are true than DO NOT DO ANYTHING because this means
 *     xeliminates might just consist of "anything" telegrams that
 *     do not specify enough data to conclude anything from it!
 */
static char dcf77_timelayer_cross_check_by_xeliminate(
	struct dcf77_timelayer* ctx, struct dcf77_secondlayer* secondlayer)
{
	struct dcf77_timelayer_tm current_plus_one; 
	unsigned char virtual_telegram_plus_1_min[DCF77_SECONDLAYER_LINE_BYTES];
	char eliminates_for_one_minute_back;
	char eliminates_for_one_minute_forward;

	current_plus_one = ctx->out_current;
	dcf77_timelayer_advance_tm_by_sec(&current_plus_one, 60);
	dcf77_timelayer_tm_to_telegram(&current_plus_one,
						virtual_telegram_plus_1_min);

	eliminates_for_one_minute_forward = dcf77_secondlayer_xeliminate(0,
		secondlayer->out_telegram_1, virtual_telegram_plus_1_min);

	eliminates_for_one_minute_back = dcf77_secondlayer_xeliminate(0,
		ctx->private_prev_telegram, secondlayer->out_telegram_1);

	if(eliminates_for_one_minute_forward && eliminates_for_one_minute_back)
		/* 5C */
		return 0;

	if(eliminates_for_one_minute_back) {
		/* 5A */
		ctx->out_current = ctx->private_prev;
		ctx->private_num_seconds_since_prev =
						DCF77_TIMELAYER_PREV_UNKNOWN;
		ctx->out_qos = DCF77_TIMELAYER_QOS7; /* backward */
		return 1;
	}
	if(eliminates_for_one_minute_forward) {
		/* 5B */
		ctx->out_current = current_plus_one;
		ctx->private_num_seconds_since_prev = 
						DCF77_TIMELAYER_PREV_UNKNOWN;
		ctx->out_qos = DCF77_TIMELAYER_QOS8; /* forward */
		return 1;
	}

	/* else none of 5 cases matches */
	return 0;
}
