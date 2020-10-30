#include "dcf77_bitlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_secondlayer.h"
#include "dcf77_nextl.h"
#include "dcf77_proc_moventries.h"

/* TODO DEBUG ONLY */
static void printtel_sub(unsigned char* data)
{
	unsigned char j;
	for(j = 0; j < 15; j++)
		printf("%02x,", data[j]);
	printf("(%02x)", data[15]);
	putchar('\n');
}

static void dumpmem(struct dcf77_secondlayer* ctx)
{
	unsigned char i;
	printf("    [DEBUG]         ");
	for(i = 0; i < DCF77_SECONDLAYER_LINE_BYTES; i++) {
		if((ctx->private_line_cursor/4) == i) {
			if(ctx->private_line_cursor % 4 >= 2)
				printf("*  ");
			else
				printf(" * ");
		} else {
			printf("   ");
		}
	}
	putchar('\n');
	for(i = 0; i < DCF77_SECONDLAYER_LINES; i++) {
		printf("    [DEBUG] %s meml%d=", i == ctx->private_line_current?
								"*": " ", i);
		printtel_sub(ctx->private_telegram_data +
					(i * DCF77_SECONDLAYER_LINE_BYTES));
	}
	printf("    [DEBUG] line_current=%u, cursor=%u\n",
			ctx->private_line_current, ctx->private_line_cursor);
	printf("    [DEBUG] out_telegram_1 len=%2u: ", ctx->out_telegram_1_len);
	printtel_sub(ctx->out_telegram_1);
	printf("    [DEBUG] out_telegram_2 len=%2u: ", ctx->out_telegram_2_len);
	printtel_sub(ctx->out_telegram_2);
}
/* END DEBUG ONLY */

void dcf77_proc_recompute_eom(struct dcf77_secondlayer* ctx)
{
	/*
	 * Precondition: Current line is "full", but does not end on eom.
	 * Case length = 60: regular minute
	 * Case length = 61: expected a leap second but did not get NO_SIGNAL
	 *                   marker.
	 * Needs to
	 * (1) forward-identify a new eom marker starting from the
	 * end of the current first entry [which effectively means current+1
	 * w/ skip empty]
	 * (2) once identified, move all bits backwards the difference between
	 * the old eom and the newly identified eom. This way, some bits
	 * get shifted off at the first entry and thus the amount of data
	 * lessens. This should re-use existing work on leftward shifting.
	 * (3) hand back to process_telegrams: this is not as trivial as it
	 * may sound. the problem is: there is actually no new telegram until
	 * the new "current" minute has finished. Thus will usually end there
	 * w/o returning new telegram data [can there be different behaviour in
	 * the presence of leap seconds? Not so easy to answer...]
	 */

	puts("    CALLING recompute_eom()");
	/*dumpmem(ctx);*/

	/* -- Step 1: Identify new eom position -- */

	/*
	 * again two bits per entry:
	 * 11 = no   mism good
	 * 10 = one  mism good if leap
	 * 01 = two+ mism never good
	 * 00 = (error)   too many decrements = bug = should not happen
	 */
	unsigned char current_valid_eom_options[DCF77_SECONDLAYER_LINE_BYTES];
	unsigned char line;
	unsigned char curbit;

	unsigned char curval;
	unsigned char curopt;

	/* DCF77_SECONDLAYER_LINES means none here (impossible index) */
	unsigned char in_line_with_leapsec = DCF77_SECONDLAYER_LINES;

	/* unsigned char i; * TODO DEBUG ONLY */

	/*
	 * Initially, all positions are potential new eoms...
	 * This version is not particulary optimized for performance but
	 * might do just OK.
	 *
	 * Idea: Two bits for each position.
	 *       Let 3 = 0b11  good
	 *       Let 2 = 0b10  good [in event of leap second/single mismatch]
         *       Let 1 = 0b01  bad, not useable for new EOM position!
	 *       Let 0 = 0b00  never occurs
	 */
	memset(current_valid_eom_options, 0xff,
					sizeof(current_valid_eom_options));

	/* BEGIN DEBUG */
	/*
	printf("     recompute_eom(1) ");
	for(i = 0; i < sizeof(current_valid_eom_options); i++)
		printf("%02x", current_valid_eom_options[i]);
	putchar('\n');
	*/
	/* END DEBUG */

	/*
	 * New 2020/10/22: Loop now includes considering the current line
	 *                 because it needs to match the scheme, too!
	 */
	line = ctx->private_line_current;
	do {
		line = dcf77_nextl(line);

		/*printf("## line = %d\n", line); DEBUG ONLY*/

		/* skip empty lines */
		if(ctx->private_line_lengths[line] == 0)
			continue;

		for(curbit = 0; curbit < ctx->private_line_lengths[line];
								curbit++) {
			curopt = dcf77_telegram_read_bit(curbit,
						current_valid_eom_options);
			/* DEBUG ONLY printf(".%d", curopt); */
			if(curopt == 1) /* no chance to use this, skip */
				continue;

			curval = dcf77_telegram_read_bit(
				curbit,
				ctx->private_telegram_data + (line *
						DCF77_SECONDLAYER_LINE_BYTES)
			);
			/* DEBUG ONLY printf("[%d]", curval); */
			if(curval == DCF77_BIT_1)
				/* a place with 1 is never accepted */
				dcf77_telegram_write_bit(curbit,
					current_valid_eom_options, 1);
			else if(curval == DCF77_BIT_0)
				dcf77_telegram_write_bit(curbit,
					current_valid_eom_options, curopt - 1);
		}

		/* printf("\n## end line = %d\n", line); DEBUG ONLY */
	/* arrived at last line to process which is current. Loop terminates */
	} while(line != ctx->private_line_current);

	/* BEGIN DEBUG */
	/*
	printf("     recompute_eom(2) ");
	for(i = 0; i < sizeof(current_valid_eom_options); i++)
		printf("%02x", current_valid_eom_options[i]);
	putchar('\n');
	for(i = 0; i < 60; i++) {
		curopt = dcf77_telegram_read_bit(i, current_valid_eom_options);
		printf(".%d", curopt);
	}
	putchar('\n');
	*/
	/* END DEBUG */

	/* -- Step 2: Check new eom position -- */
	for(curbit = 0; curbit < 60; curbit++) {
		curval = dcf77_telegram_read_bit(curbit,
						current_valid_eom_options);
		if(curval == 3)
			break; /* there it is */
	}
	if(curbit == 60 && ctx->private_leap_second_expected != 0) {
		for(curbit = 0; curbit < 60; curbit++) {
			curval = dcf77_telegram_read_bit(curbit,
						current_valid_eom_options);
			if(curval == 2) {
				in_line_with_leapsec = line;
				break; /* OK, leap sec, this might be valid */
			}
		}
	}
	if(curbit == 60) {
		/*
		 * nothing was found. this should normally be impossible.
		 * Do we have some bogus reading or attack. Or program
		 * bug (more likely). In any case, there is nothing but
		 * a reset to solve this data inconsistency
		 */
		/* reset(ctx); */
		/* TODO RESET DISABLED FOR DEBUG ONLY */
		printf("reset(ctx)\n");
		exit(64);
		return;
	}

	/* -- Step 3: Apply new eom position -- */

	printf("    move_entries_backwards(ctx, curbit=%u, "
		"in_line_with_leapsec=%u)\n", curbit, in_line_with_leapsec);
	dumpmem(ctx); /* DEBUG ONLY */
	/*
	 * Everyone needs to move curbit steps to the left. This honors
	 * the length of lines and considers the case of a reduction of the
	 * total number of lines.
	 */
	dcf77_proc_move_entries_backwards(ctx, curbit + 1,
							in_line_with_leapsec);
	dumpmem(ctx); /* DEBUG ONLY */
	puts("    END recompute_eom()");
}
