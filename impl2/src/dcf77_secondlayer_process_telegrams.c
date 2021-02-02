#include <string.h>

#include <stdio.h> /* TODO DEBUG ONLY */

#include "dcf77_bitlayer.h"
#include "dcf77_offsets.h"
#include "dcf77_telegram.h"

#include "dcf77_secondlayer.h"
#include "dcf77_line.h"
#include "dcf77_secondlayer_xeliminate.h"
#include "dcf77_secondlayer_process_telegrams.h"
#include "dcf77_secondlayer_recompute_eom.h"

/* ================================================== private declarations == */

/* should all fit within two bytes (like returning an integer...) */
struct dcf77_secondlayer_process_telegrams_merge_result {
	char           match:           1; /* bool */
	char           is_leap_in_line: 1; /* bool */
	unsigned char  line:            6; /* index (need four bits minimum) */
	unsigned char* ptr_to_line;
};

static struct dcf77_secondlayer_process_telegrams_merge_result
dcf77_secondlayer_process_telegrams_try_merge(struct dcf77_secondlayer* ctx,
		unsigned char line_current, char backup_before_eliminate);
static void dcf77_secondlayer_process_telegrams_advance_to_next_line(
						struct dcf77_secondlayer* ctx);
static void dcf77_secondlayer_process_telegrams_postprocess(
		struct dcf77_secondlayer* ctx, unsigned char* in_out_telegram,
		unsigned char* in_telegram);
static void dcf77_secondlayer_process_telegrams_add_missing_bits(
		unsigned char* in_out_telegram, unsigned char* in_telegram);
static void dcf77_secondlayer_process_telegrams_check_for_leapsec_announce(
			struct dcf77_secondlayer* ctx, unsigned char* telegram);

/* ================================================== begin implementation == */

void dcf77_secondlayer_process_telegrams(struct dcf77_secondlayer* ctx)
{
	struct dcf77_secondlayer_process_telegrams_merge_result rv;

	/* Input situation: cursor is at the end of the current minute. */

	/* first clear buffers to no signal */
	memset(ctx->out_telegram_1, 0x55, DCF77_SECONDLAYER_LINE_BYTES);
	memset(ctx->out_telegram_2, 0x55, DCF77_SECONDLAYER_LINE_BYTES);

	/* merge till mismatch */
	rv = dcf77_secondlayer_process_telegrams_try_merge(ctx,
						ctx->private_line_current, 1);

	if(rv.match) {
		dcf77_secondlayer_process_telegrams_postprocess(ctx,
					ctx->out_telegram_1, rv.ptr_to_line);
		/* that is already the actual output */
		ctx->out_telegram_1_len = (rv.is_leap_in_line? 61: 60);
		ctx->out_telegram_2_len = 0;
		dcf77_secondlayer_process_telegrams_advance_to_next_line(ctx);
		/* return */
	} else {
		ctx->out_telegram_2_len = 60; /* generically write 60... */

		/*
		 * repeat and write to actual output
		 * line is now one before the line that failed and which we
		 * reprocess.
		 */
		rv = dcf77_secondlayer_process_telegrams_try_merge(
					ctx, ((rv.line == 0)?
						(DCF77_SECONDLAYER_LINES - 1):
						(rv.line - 1)), 0);

		if(rv.match) {
			/*
			 * no further mismatch. Data in the buffer is fully
			 * consistent. Can output this as truth
			 */
			ctx->out_telegram_1_len = (rv.is_leap_in_line? 61: 60);
			dcf77_secondlayer_process_telegrams_postprocess(ctx,
					ctx->out_telegram_1, rv.ptr_to_line);
			dcf77_secondlayer_process_telegrams_advance_to_next_line
									(ctx);
			/* return */
		} else {
			/*
			 * we got another mismatch. this means the data is
			 * not consistent.
			 */
			ctx->out_telegram_1_len = 0;
			ctx->out_telegram_2_len = 0;
			puts("    recompute_eom because: telegram processing mismatch. TODO INCOMPLETE IMPLEMENTATION SEE SOURCE CODE");
			dcf77_secondlayer_recompute_eom(ctx);
			/* TODO re-invoke as described on paper. Remember that this has to advance line... */
		}
	}
}

static struct dcf77_secondlayer_process_telegrams_merge_result
	dcf77_secondlayer_process_telegrams_try_merge(
		struct dcf77_secondlayer* ctx, unsigned char line_current,
		char backup_before_eliminate)
{
	unsigned char match = 1;
	unsigned char line = line_current;
	/* init to 0 to avoid warning of uninitialized */
	unsigned char is_leap_in_line = 0;
	unsigned char* ptr_to_line;
	struct dcf77_secondlayer_process_telegrams_merge_result rv;

	do {
		line = dcf77_line_next(line);
		ptr_to_line = dcf77_line_pointer(ctx, line);

		/* ignore empty lines */
		if(dcf77_line_is_empty(ptr_to_line))
			continue;

		/* backup contents before processing */
		if(backup_before_eliminate)
			memcpy(ctx->out_telegram_2, ctx->out_telegram_1,
						DCF77_SECONDLAYER_LINE_BYTES);

		rv.is_leap_in_line = (line == ctx->private_leap_in_line);
		match = dcf77_secondlayer_xeliminate(rv.is_leap_in_line,
					ptr_to_line, ctx->out_telegram_1);

	} while((line != ctx->private_line_current) && match);

	rv.match           = match;
	rv.is_leap_in_line = is_leap_in_line;
	rv.line            = line;
	return rv;
}

/* includes clearing the next line's contents */
static void dcf77_secondlayer_process_telegrams_advance_to_next_line(
						struct dcf77_secondlayer* ctx)
{
	ctx->private_line_cursor = 0;
	ctx->private_line_current = dcf77_line_next(ctx->private_line_current);
	/* clear line by setting last bit to unset */
	dcf77_telegram_write_bit(DCF77_OFFSET_ENDMARKER_REGULAR,
		ctx->private_telegram_data + (ctx->private_line_current *
			DCF77_SECONDLAYER_LINE_BYTES), DCF77_BIT_NO_UPDATE);
}

static void dcf77_secondlayer_process_telegrams_postprocess(
		struct dcf77_secondlayer* ctx, unsigned char* in_out_telegram,
		unsigned char* in_telegram)
{
	dcf77_secondlayer_process_telegrams_add_missing_bits(in_out_telegram,
							in_telegram);
	dcf77_secondlayer_process_telegrams_check_for_leapsec_announce(ctx,
							in_out_telegram);
}

/*
 * this is not a "proper" X-elimination but copies minute value bits,
 * leap second announce bit and minute parity from the last telegram processed
 * prior to outputting something. It allows the higher layers to receive and
 * process these bits although they are not set by the normal xelimination.
 */
static void dcf77_secondlayer_process_telegrams_add_missing_bits(
		unsigned char* in_out_telegram, unsigned char* in_telegram)
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

static void dcf77_secondlayer_process_telegrams_check_for_leapsec_announce(
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
