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
	unsigned short private_leap_second_expected;
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
static void shift_existing_bits_to_the_left(struct dcf77_high_level* ctx);

void dcf77_high_level_init(struct dcf77_high_level* ctx)
{
	/* this is actually the same as a good-old reset function */
	ctx->private_inmode          = IN_BACKWARD;
	ctx->private_line_lengths[0] = 0;
	ctx->private_line_current    = 0;
	ctx->private_line_cursor     = 59;
	ctx->private_leap_second_expected = 0; /* no leap second expected */
	ctx->out_has_new_telegram    = 0;
	/* initialize with epsilon */
	memset(ctx->private_telegram_data, 0, DCF77_HIGH_LEVEL_MEM);
}

void dcf77_high_level_process(struct dcf77_high_level* ctx)
{
	/* do nothing if no update */
	if(ctx->in_val == DCF77_BIT_NO_UPDATE)
		return;

	/* write new input */
	ctx->private_telegram_data[ctx->private_line_cursor / 4] |= 
			(ctx->in_val << ((ctx->private_line_cursor % 4) * 2));
	ctx->private_line_lengths[ctx->private_line_current]++;

	/* decrease leap second expectation */
	if(ctx->private_leap_second_expected != 0)
		ctx->private_leap_second_expected--;

	/* automaton case-specific handling */
	switch(ctx->private_inmode) {
	case IN_BACKWARD:
		if(ctx->in_val == DCF77_BIT_NO_SIGNAL) {
			/*
			 * no signal might indicate: end of minute.
			 * let us thus start a new line and switch to aligned
			 * mode without moving bits.
			 */
			ctx->private_inmode          = IN_FORWARD;
			ctx->private_line_current    = 1;
			ctx->private_line_cursor     = 0;
			ctx->private_line_lengths[1] = 0;
		} else if(ctx->private_line_lengths[0] == 60) {
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
			shift_existing_bits_to_the_left(ctx);
		}
		break;
	case IN_FORWARD:
		if(ctx->private_line_cursor == 59) {
			/*
			 * we just wrote the 60. bit (at index 59).
			 * this means we are at the end of a minute.
			 * the current value should reflect this or
			 * it might possibly be a leap second (if announced).
			 */
			if(ctx->in_val == DCF77_BIT_NO_SIGNAL) {
				/*
				 * no signal in any case means this is our
				 * end-of-minute marker
				 */
				/* TODO N_IMPL invoke process_telegrams() and advance to next "line" */
			} else if(ctx->in_val == DCF77_BIT_0 &&
					ctx->private_leap_second_expected > 0) {
				/*
				 * this was possibly the expected leap second
				 * which is marked by an additional 0-bit.
				 * In this very case, the cursor position 60
				 * becomes available for processing.
				 */
				ctx->private_line_cursor++;
			} else {
				/*
				 * A signal was received but in this specific
				 * data arragngement, there should definitely
				 * have been a `NO_SIGNAL` at this place.
				 * We need to reorganize the datastructure
				 * to align to the "reality".
				 */
				/* TODO N_IMPL / invoke recompute_eom(), afterwards the line will likely not be 100% full, but cursor reorganization is handled by compute_eom... */
			}
		} else {
			/*
			 * If we are not near the end, just quietly append
			 * received bits, i.e. advance cursor (the rest of
			 * that is already implemented above).
			 */
			ctx->private_line_cursor++;
		}
		break;
	}
}

static void shift_existing_bits_to_the_left(struct dcf77_high_level* ctx)
{
	unsigned char current_byte;

	/* TODO z not sure if this -1 makes sense here */
	for(current_byte = (60 - ctx->private_line_lengths[0] - 1) / 4;
					current_byte < 16; current_byte++) {
		/*
		 * split off the first two bits (which get shifted out)
		 * and put them in the previous byte if there is a previous
		 * byte (otherwise discard them silently).
		 */
		if(current_byte > 0)
			ctx->private_telegram_data[current_byte - 1] |= (
				(ctx->private_telegram_data[current_byte] &
				0xc0) >> 6);
		/*
		 * shift current byte two leftwards, making space for one
		 * datapoint
		 */
		ctx->private_telegram_data[current_byte] <<= 2;
	}
}
