#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_nextl.h"
#include "dcf77_proc_process_telegrams.h"
#include "dcf77_proc_xeliminate.h"
#include "dcf77_proc_recompute_eom.h"

static inline unsigned char prevl(unsigned char inl);
static void advance_to_next_line(struct dcf77_secondlayer* ctx);
static void postprocess(struct dcf77_secondlayer* ctx,
		unsigned char* in_out_telegram, unsigned char* in_telegram);
static void add_missing_bits(unsigned char* in_out_telegram,
						unsigned char* in_telegram);
static void check_for_leapsec_announce(struct dcf77_secondlayer* ctx,
						unsigned char* telegram);

void dcf77_proc_process_telegrams(struct dcf77_secondlayer* ctx)
{
	unsigned char lastlen = 60;
	unsigned char line;
	char match;

	/* Input situation: cursor is at the end of the current minute. */

	/* first clear buffers to no signal */
	memset(ctx->out_telegram_1, 0x55, DCF77_SECONDLAYER_LINE_BYTES);
	memset(ctx->out_telegram_2, 0x55, DCF77_SECONDLAYER_LINE_BYTES);

	/* merge till mismatch */
	/* TODO z printf("    [DEBUG] FROM %u to %u\n", nextl(ctx->private_line_current + 1), ctx->private_line_current); */
	for(match = 1, line = dcf77_nextl(ctx->private_line_current);
				line != ctx->private_line_current && match;
				line = dcf77_nextl(line)) {
		/* ignore empty lines */
		if(ctx->private_line_lengths[line] == 0)
			continue;

		/*
		printf("    [DEBUG] xeliminate1(\n");
		printf("    [DEBUG]   ");
		printtel_sub(ctx->private_telegram_data + 
					(DCF77_SECONDLAYER_LINE_BYTES * line));
		printf("    [DEBUG]   ");
		printtel_sub(ctx->out_telegram_1);
		*/
		match = dcf77_proc_xeliminate(ctx->private_line_lengths[line],
					lastlen, ctx->private_telegram_data +
					(DCF77_SECONDLAYER_LINE_BYTES * line),
					ctx->out_telegram_1);
		/*
		printf("    [DEBUG] )=%d\n", match);
		*/
		lastlen = ctx->private_line_lengths[line];
	}

	postprocess(ctx, ctx->out_telegram_1,
					ctx->private_telegram_data +
					(DCF77_SECONDLAYER_LINE_BYTES * line));

	if(match) {
		/* that was already the actual output */
		ctx->out_telegram_1_len = lastlen;
		ctx->out_telegram_2_len = 0;
		advance_to_next_line(ctx);
		/* return */
	} else {
		/* repeat and write to actual output */

		/* this is the line that failed and which we reprocess */
		line = prevl(line);
		/* this is the length of the line before the line that failed */
		ctx->out_telegram_1_len =
					ctx->private_line_lengths[prevl(line)];

		match = 1;
		/* we eliminate to generic thus set length to 60 */
		lastlen = 60;
		for(; line != ctx->private_line_current && match;
						line = dcf77_nextl(line)) {
			/* ignore empty lines (relevant in the beginning) */
			if(ctx->private_line_lengths[line] == 0)
				continue;

			/*
			printf("    [DEBUG] xeliminate2(\n");
			printf("    [DEBUG]   ");
			printtel_sub(ctx->private_telegram_data + 
					(DCF77_SECONDLAYER_LINE_BYTES * line));
			printf("    [DEBUG]   ");
			printtel_sub(ctx->out_telegram_2);
			*/
			match = dcf77_proc_xeliminate(
					ctx->private_line_lengths[line],
					lastlen, ctx->private_telegram_data +
					(DCF77_SECONDLAYER_LINE_BYTES * line),
					ctx->out_telegram_2);
			/*
			printf("    [DEBUG] )=%d\n", match);
			*/
			lastlen = ctx->private_line_lengths[line];
		}

		if(match) {
			/*
			 * no further mismatch. Data in the buffer is fully
			 * consistent. Can output this as truth
			 */
			ctx->out_telegram_2_len = lastlen;
			postprocess(ctx, ctx->out_telegram_2,
					ctx->private_telegram_data +
					(DCF77_SECONDLAYER_LINE_BYTES * line));
			advance_to_next_line(ctx);
			/* return */
		} else {
			/*
			 * we got another mismatch. this means the data is
			 * not consistent.
			 */
			ctx->out_telegram_1_len = 0;
			ctx->out_telegram_2_len = 0;
			puts("    recompute_eom because: telegram processing mismatch. TODO INCOMPLETE IMPLEMENTATION SEE SOURCE CODE");
			dcf77_proc_recompute_eom(ctx);
			/* TODO re-invoke as described on paper. Remember that this has to advance line... */
		}
	}
}

/* includes clearing the next line's contents */
static void advance_to_next_line(struct dcf77_secondlayer* ctx)
{
	ctx->private_line_cursor = 0;
	ctx->private_line_current = dcf77_nextl(ctx->private_line_current);
	ctx->private_line_lengths[ctx->private_line_current] = 0;
	memset(ctx->private_telegram_data +
		(ctx->private_line_current * DCF77_SECONDLAYER_LINE_BYTES),
		0, DCF77_SECONDLAYER_LINE_BYTES);
}

static inline unsigned char prevl(unsigned char inl)
{
	return ((inl == 0)? (DCF77_SECONDLAYER_LINES - 1): (inl - 1));
}

static void postprocess(struct dcf77_secondlayer* ctx,
		unsigned char* in_out_telegram, unsigned char* in_telegram)
{
	add_missing_bits(in_out_telegram, in_telegram);
	check_for_leapsec_announce(ctx, in_out_telegram);
}

/*
 * this is not a "proper" X-elimination but copies minute value bits and
 * leap second announce bits from the last telegram processed prior to
 * outputting something. It allows the higher layers to receive and process
 * these bits although they are not used in the normal xelimination.
 */
static void add_missing_bits(unsigned char* in_out_telegram,
						unsigned char* in_telegram)
{
	unsigned char i;
	dcf77_proc_xeliminate_entry(in_telegram[19 / 4],
					in_out_telegram + (19 / 4), 19 % 4);
	for(i = 21; i <= 24; i++)
		dcf77_proc_xeliminate_entry(in_telegram[i / 4],
					in_out_telegram + (i / 4), i % 4);
}

static void check_for_leapsec_announce(struct dcf77_secondlayer* ctx,
							unsigned char* telegram)
{
	/*
	 * leapsec bit idx = 19. 19/4 = 4, 19%4 = 3 (slot idx 3 [0..3])
	 * to read third slot we use uppermost two bits: 0xc0 = 1100 0000
	 * then these need to match "bit 1" which is 11 thus byte&0xc0 = 0xc0
	 * indicates leap second marker activated!
	 */

	/* find leap sec marker && only set if no counter in place yet */
	if((telegram[4] & 0xc0) == 0xc0 &&
				ctx->private_leap_second_expected == 0) {
		/*
		 * leap sec lies at most one hour in the future. we allow +10
		 * minutes s.t. existing telegrams (w/ leap sec) do not
		 * become invalid until the whole telegram from the leap
		 * second has passed out of memory.
		 */
		ctx->private_leap_second_expected = 70 * 60;
	}
}
