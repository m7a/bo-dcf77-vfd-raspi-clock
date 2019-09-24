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

enum dcf77_high_level_input_mode { IN_INIT, IN_ALIGNED, IN_UNKNOWN };

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
	char out_has_new_second;
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
	ctx->private_inmode          = IN_INIT;
	ctx->private_line_lengths[0] = 0;
	ctx->private_line_current    = 0;
	ctx->private_line_cursor     = 0;
	ctx->out_has_new_second      = 0;
	ctx->out_has_new_telegram    = 0;
}

void dcf77_high_level_process(struct dcf77_high_level* ctx)
{
	switch(ctx->private_inmode) {
	case IN_INIT:
		break;
	case IN_ALIGNED:
		break;
	case IN_UNKNOWN:
		break;
	}
}
