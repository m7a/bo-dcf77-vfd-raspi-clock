#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_offsets.h"
#include "dcf77_telegram.h"

#include "dcf77_secondlayer.h"
#include "dcf77_line.h"
#include "dcf77_secondlayer_xeliminate.h"
#include "dcf77_secondlayer_check_bcd_correct_telegram.h"
#include "dcf77_secondlayer_recompute_eom.h"
#include "dcf77_secondlayer_process_telegrams.h"

/* ================================================== private declarations == */

/* should all fit within two bytes (like returning an integer...) */
struct process_telegrams_merge_result {
	char           match:           1; /* bool */
	char           is_leap_in_line: 1; /* bool */
	unsigned char  line:            6; /* index (need four bits minimum) */
	unsigned char* ptr_to_line;
};

static void process_telegrams_correct_minute(struct dcf77_secondlayer* ctx);
static struct process_telegrams_merge_result process_telegrams_try_merge(
		struct dcf77_secondlayer* ctx, unsigned char line_current,
		char backup_before_eliminate);
static void process_telegrams_no_mismatch(struct dcf77_secondlayer* ctx,
				struct process_telegrams_merge_result* rv);
static void process_telegrams_postprocess(struct dcf77_secondlayer* ctx,
		unsigned char* in_out_telegram, unsigned char* in_telegram);
static void process_telegrams_add_missing_bits(unsigned char* in_out_telegram,
						unsigned char* in_telegram);
static void process_telegrams_check_for_leapsec_announce(
			struct dcf77_secondlayer* ctx, unsigned char* telegram);
static void process_telegrams_single_mismatch(struct dcf77_secondlayer* ctx,
							unsigned char line);

/* ================================================== begin implementation == */

void dcf77_secondlayer_process_telegrams(struct dcf77_secondlayer* ctx)
{

	/* Input situation: cursor is at the end of the current minute. */

	if(dcf77_secondlayer_check_bcd_correct_telegram_ignore_eom(ctx,
						ctx->private_line_current, 0)) {
		process_telegrams_correct_minute(ctx);
	} else {
		/*
		 * received data is invalid. This requires us to perform a
		 * recompute_eom()
		 */
		ctx->out_telegram_1_len = 0;
		ctx->out_telegram_2_len = 0;
		dcf77_secondlayer_recompute_eom(ctx);
	}
}

static void process_telegrams_correct_minute(struct dcf77_secondlayer* ctx)
{
	struct process_telegrams_merge_result rv;

	/* first clear buffers to no signal */
	memset(ctx->out_telegram_1, 0x55, DCF77_SECONDLAYER_LINE_BYTES);
	memset(ctx->out_telegram_2, 0x55, DCF77_SECONDLAYER_LINE_BYTES);

	/* merge till mismatch */
	rv = process_telegrams_try_merge(ctx, ctx->private_line_current, 1);
	if(rv.match)
		process_telegrams_no_mismatch(ctx, &rv);
	else
		process_telegrams_single_mismatch(ctx, rv.line);
}

static struct process_telegrams_merge_result process_telegrams_try_merge(
		struct dcf77_secondlayer* ctx, unsigned char line_current,
		char backup_before_eliminate)
{
	struct process_telegrams_merge_result rv = {
		.match           = 1,
		.line            = line_current,
		/* init to 0 to avoid warning of uninitialized */
		.is_leap_in_line = 0,
	};

	do {
		rv.line = dcf77_line_next(rv.line);
		rv.ptr_to_line = dcf77_line_pointer(ctx, rv.line);

		/* ignore empty lines */
		if(dcf77_line_is_empty(rv.ptr_to_line))
			continue;

		/* backup contents before processing */
		if(backup_before_eliminate)
			memcpy(ctx->out_telegram_2, ctx->out_telegram_1,
						DCF77_SECONDLAYER_LINE_BYTES);

		rv.is_leap_in_line = (rv.line == ctx->private_leap_in_line);
		rv.match = dcf77_secondlayer_xeliminate(rv.is_leap_in_line,
					rv.ptr_to_line, ctx->out_telegram_1);

	} while((rv.line != ctx->private_line_current) && rv.match);

	return rv;
}

/*
 * No mismatch at all means: No minute tens change in buffer.
 * E.g. 13:00, 13:01, 13:02, 13:03, 13:04, ... 13:08
 */
static void process_telegrams_no_mismatch(struct dcf77_secondlayer* ctx,
				struct process_telegrams_merge_result* rv)
{
	process_telegrams_postprocess(ctx,
					ctx->out_telegram_1, rv->ptr_to_line);
	/* that is already the actual output */
	ctx->out_telegram_1_len = (rv->is_leap_in_line? 61: 60);
	ctx->out_telegram_2_len = 0;
	if(!rv->is_leap_in_line)
		dcf77_secondlayer_process_telegrams_advance_to_next_line(ctx);
}

static void process_telegrams_postprocess(struct dcf77_secondlayer* ctx,
		unsigned char* in_out_telegram, unsigned char* in_telegram)
{
	process_telegrams_add_missing_bits(in_out_telegram, in_telegram);
	process_telegrams_check_for_leapsec_announce(ctx, in_out_telegram);
}

/*
 * this is not a "proper" X-elimination but copies minute value bits,
 * leap second announce bit and minute parity from the last telegram processed
 * prior to outputting something. It allows the higher layers to receive and
 * process these bits although they are not set by the normal xelimination.
 */
static void process_telegrams_add_missing_bits(unsigned char* in_out_telegram,
						unsigned char* in_telegram)
{
	unsigned char i;

	dcf77_secondlayer_xeliminate_entry(in_telegram, in_out_telegram,
						DCF77_OFFSET_LEAP_SEC_ANNOUNCE);

	for(i = DCF77_OFFSET_MINUTE_ONES;
					i < (DCF77_OFFSET_MINUTE_ONES + 4); i++)
		dcf77_secondlayer_xeliminate_entry(in_telegram,
							in_out_telegram, i);

	dcf77_secondlayer_xeliminate_entry(in_telegram, in_out_telegram,
						DCF77_OFFSET_PARITY_MINUTE);
}

static void process_telegrams_check_for_leapsec_announce(
			struct dcf77_secondlayer* ctx, unsigned char* telegram)
{
	/* Only set if no counter in place yet. */
	if(dcf77_telegram_read_bit(DCF77_OFFSET_LEAP_SEC_ANNOUNCE, telegram) ==
			DCF77_BIT_1 && ctx->private_leap_second_expected == 0) {
		/*
		 * leap sec lies at most one hour in the future. we allow +10
		 * minutes s.t. existing telegrams (w/ leap sec) do not
		 * become invalid until the whole telegram from the leap
		 * second has passed out of memory.
		 */
		ctx->private_leap_second_expected = 70 * 60;
	}
}

/* includes clearing the next line's contents */
void dcf77_secondlayer_process_telegrams_advance_to_next_line(
						struct dcf77_secondlayer* ctx)
{
	ctx->private_line_cursor = 0;
	ctx->private_line_current = dcf77_line_next(ctx->private_line_current);
	/* clear line by setting last bit to unset */
	/* TODO z DOES THAT ACTUALLY WORK GIVEN THAT is_empty CHECKS THE _FIRST_ BIT AND NOT THE LAST ONE? */
	dcf77_telegram_write_bit(DCF77_OFFSET_ENDMARKER_REGULAR,
		ctx->private_telegram_data + (ctx->private_line_current *
			DCF77_SECONDLAYER_LINE_BYTES), DCF77_BIT_NO_UPDATE);
}

/*
 * Single mismatch means: Minute tens change in buffer. This may at most happen
 * once. E.g.: 13:05, 13:06, 13:07, 13:08, 13:09, 13:10, ... 13:13
 */
static void process_telegrams_single_mismatch(struct dcf77_secondlayer* ctx,
							unsigned char line)
{
	struct process_telegrams_merge_result rv;

	/*
	 * Second telegram now contains previous minute (the data from begin of
	 * buffer up until mismatch exclusive) generically write 60...
	 */
	ctx->out_telegram_2_len = 60;

	/*
	 * clear mismatching out_telegram_1, otherwise the xeliminates in
	 * try_merge might fail.
	 */
	memset(ctx->out_telegram_1, 0x55, DCF77_SECONDLAYER_LINE_BYTES);

	/*
	 * repeat and write to actual output line is now one before the line
	 * that failed and which we reprocess.
	 */
	rv = process_telegrams_try_merge(ctx, dcf77_line_prev(line), 0);

	if(rv.match) {
		/*
		 * no further mismatch. Data in the buffer is fully consistent.
		 * Can output this as truth.
		 */
		ctx->out_telegram_1_len = (rv.is_leap_in_line? 61: 60);
		process_telegrams_postprocess(ctx, ctx->out_telegram_1,
								rv.ptr_to_line);
		
		/*
		 * Do not advance cursor if we have a leap second because cursor
		 * will stay in same line and reach index 60 next second.
		 */
		if(!rv.is_leap_in_line)
			dcf77_secondlayer_process_telegrams_advance_to_next_line
									(ctx);
	} else {
		/*
		 * we got another mismatch. The data is not consistent.
		 */
		ctx->out_telegram_1_len = 0;
		ctx->out_telegram_2_len = 0;
		dcf77_secondlayer_recompute_eom(ctx);
	}
}
