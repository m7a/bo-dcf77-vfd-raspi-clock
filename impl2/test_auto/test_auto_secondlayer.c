#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_proc_xeliminate.h"
#include "xeliminate_testcases.h"

int main(int argc, char** argv)
{
	return EXIT_SUCCESS;
}

/* ---------------------------------------------------[ Logic Declaration ]-- */

/* interface */
#define DCF77_HIGH_LEVEL_MEM      144 /* ceil(61*2/8) * 9 */
#define DCF77_HIGH_LEVEL_LINES    9
#define DCF77_HIGH_LEVEL_TIME_LEN 8
#define DCF77_HIGH_LEVEL_DATE_LEN 10

enum dcf77_high_level_input_mode {
	/* Init mode. Push data backwards */
	IN_BACKWARD,
	/* Aligned+Unknown mode. Push data forwards */
	IN_FORWARD
};

struct dcf77_high_level {
	/* private */
	enum dcf77_high_level_input_mode private_inmode;
	unsigned char private_telegram_data[DCF77_HIGH_LEVEL_MEM];
	/*
	 * the start and end are actually fixed because if we were to write
	 * continuuously in the same manner, then some telegrams would start
	 * at offsets inside bytes. As we want to avoid this very much, there
	 * is instead the necessity to reogranize data in case a new marker
	 * is intended to be used as "end" marker. The lengths given here
	 * are in bits. The offsets of the lines are fixed which each line
	 * having 16 bytes available.
	 */
	unsigned char private_line_lengths[DCF77_HIGH_LEVEL_LINES];
	unsigned char private_line_current;
	unsigned char private_line_cursor;
	/* input */
	enum dcf77_bitlayer_reading in_val;
	/* output */
	/* users should reset out_has_ values after processing! */
	/* TODO z char out_has_new_second; has new second is actually implicit: if we have a new measurement, we also have a new second! */
	char out_has_new_telegram;
	unsigned char out_telegram_mismatch_len; /* in bits */
	unsigned char out_telegram_mismatch[16];
	unsigned char out_telegram_current_len; /* in bits */
	unsigned char out_telegram_current[16];
};

void dcf77_high_level_init(struct dcf77_high_level* ctx);
void dcf77_high_level_process(struct dcf77_high_level* ctx);

/* ------------------------------------------------[ Logic Implementation ]-- */
void dcf77_high_level_init(struct dcf77_high_level* ctx)
{
	/* this is actually the same as a good-old reset function */
	ctx->private_inmode          = IN_BACKWARD;
	ctx->private_line_lengths[0] = 0;
	ctx->private_line_current    = 0;
	ctx->private_line_cursor     = 59;
	ctx->out_has_new_telegram    = 0;
	/* initialize with epsilon */
	memset(ctx->private_telegram_data, 0, DCF77_HIGH_LEVEL_MEM);
}

void dcf77_high_level_process(struct dcf77_high_level* ctx)
{
	if(ctx->in_val == DCF77_BIT_NO_UPDATE)
		return; /* nothing if no update */

	switch(ctx->private_inmode) {
	case IN_BACKWARD:
		/* write new input */
		/*
		TODO CSTAT NOW WE HAVE THE SALAD:
		THIS SHOULD WORK, BUT: VAL NEEDS TO BE IN THE SECONDLAYERS FORMAT
		WHICH IS AS OF NOW DIFFERENT TO THE LOW_LEVEL FORMAT. ALIGN THESE
		TWO FORMATS S.T. WE CAN DIRECTLY IMPORT THE DATA HERE!!!
		*/
		ctx->private_telegram_data[ctx->private_line_cursor / 4] |= 
			(ctx->in_val << ((ctx->private_line_cursor % 4) * 2));
		if(ctx->in_val == DCF77_BIT_NO_SIGNAL) {
			/*
			 * no signal might indicate: end of minute.
			 * let us thus start a new line and switch to aligned
			 * mode without moving bits.
			 */
			ctx->private_inmode          = IN_FORWARD;
			ctx->private_line_current    = 1;
			ctx->private_line_cursor     = 0;
			ctx->private_line_lengths[0]++;
			ctx->private_line_lengths[1] = 0;
		} else if(ctx->private_line_lengths[0] == 59) {
			/*
			 * we processed 59 bits before, this is the 60. without
			 * an end of minute. This is bad data. Discard
			 * everything and start anew. Note that this fault may
			 * occur if the clock is turned on just at the exact
			 * beginning of a minute with a leap second. The chances
			 * for this are quite low, so we can well say it is
			 * most likely a fault!
			 *
			 * TODO z NOTEWORTHY FAULT
			 */
			dcf77_high_level_init(ctx);
		} else {
			/*
			 * Now that we have added our input, move bits and
			 * increase number of bits in line.
			 */
			ctx->private_line_lengths[0]++;
			/* TODO CSTAT IMPLEMENT AND CALL MOVE BACKWARDS ROUTINE */
		}
		break;
	case IN_FORWARD:
		break;
	}
}
