#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_secondlayer.h"
#include "dcf77_proc_recompute_eom.h"
#include "dcf77_proc_process_telegrams.h"

#include "inc_sat.h"

#ifdef TEST
#define EXPORTED_FOR_TESTING
#else
#define EXPORTED_FOR_TESTING static
static void dcf77_secondlayer_reset(struct dcf77_secondlayer* ctx);
static void dcf77_secondlayer_write_new_input(struct dcf77_secondlayer* ctx);
static void dcf77_secondlayer_in_backward(struct dcf77_secondlayer* ctx);
#endif

static void dcf77_secondlayer_decrease_leap_second_expectation(
						struct dcf77_secondlayer* ctx);
static void dcf77_secondlayer_automaton_case_specific_handling(
						struct dcf77_secondlayer* ctx);
static void dcf77_secondlayer_in_forward(struct dcf77_secondlayer* ctx);

static void shift_existing_bits_to_the_left(struct dcf77_secondlayer* ctx);
static void complex_reorganization(struct dcf77_secondlayer* ctx);

void dcf77_secondlayer_init(struct dcf77_secondlayer* ctx)
{
	dcf77_secondlayer_reset(ctx);
	ctx->fault_reset = 0; /* reset number of resets "the first is free" */
}

EXPORTED_FOR_TESTING
void dcf77_secondlayer_reset(struct dcf77_secondlayer* ctx)
{
	ctx->private_inmode               = IN_BACKWARD;
	ctx->private_line_current         = 0;
	ctx->private_line_cursor          = 59;
	ctx->private_leap_second_expected = 0; /* no leap second expected */
	INC_SATURATED(ctx->fault_reset); /* track number of resets */
	memset(ctx->private_line_lengths,  0, DCF77_SECONDLAYER_LINES); /* 0 */
	memset(ctx->private_telegram_data, 0, DCF77_SECONDLAYER_MEM); /* eps */
	ctx->out_telegram_1_len = 0;
	ctx->out_telegram_2_len = 0;
}

void dcf77_secondlayer_process(struct dcf77_secondlayer* ctx)
{
	if(ctx->in_val != DCF77_BIT_NO_UPDATE) { /* do nothing if no update */
		dcf77_secondlayer_write_new_input(ctx);
		dcf77_secondlayer_decrease_leap_second_expectation(ctx);
		dcf77_secondlayer_automaton_case_specific_handling(ctx);
	}
}

EXPORTED_FOR_TESTING
void dcf77_secondlayer_write_new_input(struct dcf77_secondlayer* ctx)
{
	ctx->private_telegram_data[ctx->private_line_current *
		DCF77_SECONDLAYER_LINE_BYTES + ctx->private_line_cursor / 4] |= 
			(ctx->in_val << ((ctx->private_line_cursor % 4) * 2));
	ctx->private_line_lengths[ctx->private_line_current]++;
}

static void dcf77_secondlayer_decrease_leap_second_expectation(
						struct dcf77_secondlayer* ctx)
{
	if(ctx->private_leap_second_expected != 0)
		ctx->private_leap_second_expected--;
}

static void dcf77_secondlayer_automaton_case_specific_handling(
						struct dcf77_secondlayer* ctx)
{
	switch(ctx->private_inmode) {
	case IN_BACKWARD: dcf77_secondlayer_in_backward(ctx); break;
	case IN_FORWARD:  dcf77_secondlayer_in_forward(ctx);  break;
	}
}

EXPORTED_FOR_TESTING
void dcf77_secondlayer_in_backward(struct dcf77_secondlayer* ctx)
{
	if(ctx->in_val == DCF77_BIT_NO_SIGNAL) {
		/*
		 * no signal might indicate: end of minute.
		 * let us thus start a new line and switch to aligned
		 * mode without moving bits.
		 */
		if(ctx->private_line_lengths[0] == 60) {
			/*
			 * special case: The first telegram started with its
			 * first bit. Hence, we already have a full minute
			 * in our memory. Process it!
			 */
			printf("    process_telegrams (3)\n"); /* DEBUG ONLY */
			dcf77_proc_process_telegrams(ctx);
			if(ctx->out_telegram_1_len != 60) {
				/*
				 * Somehow the data received was not valid.
				 * This is expected to happen only in case of
				 * bytes recevied by wrong value. Existent data
				 * becomes untrustworthy. Force reset.
				 */
				dcf77_secondlayer_reset(ctx);
				return;
			}
		} else {
			/*
			 * we set the length to 60 because now the epsilons
			 * become part of the telegram.
			 */
			ctx->private_line_lengths[0] = 60;
		}
		ctx->private_inmode       = IN_FORWARD;
		ctx->private_line_current = 1;
		ctx->private_line_cursor  = 0;
	} else if(ctx->private_line_lengths[0] == 60) {
		/*
		 * we processed 59 bits before, this is the 60. without
		 * an end of minute. This is bad data. Discard
		 * everything and start anew. Note that this fault may
		 * occur if the clock is turned on just at the exact
		 * beginning of a minute with a leap second. The chances
		 * for this are quite low, so we can well say it is
		 * most likely a fault!
		 */
		dcf77_secondlayer_reset(ctx);
	} else {
		/*
		 * Now that we have added our input, move bits and
		 * increase number of bits in line.
		 */
		shift_existing_bits_to_the_left(ctx);
	}
}

/*
 * Although similar to the more complex shift procedure, they work on different
 * starting lines and assumptions.
 */
static void shift_existing_bits_to_the_left(struct dcf77_secondlayer* ctx)
{
	unsigned char current_byte;

	/*
	 * not sure if this -1 makes sense here /
	 * BAK
	 * for(current_byte = 15 - (ctx->private_line_lengths[0] / 4) -
	 * 			(ctx->private_line_lengths[0] % 4 != 0);
	 */
	for(current_byte = (60 - ctx->private_line_lengths[0] - 1) / 4;
				current_byte < DCF77_SECONDLAYER_LINE_BYTES;
				current_byte++) {
		/*
		 * split off the first two bits (lowermost idx,
		 * which get shifted out) and put them in the previous byte
		 * (highermost idx) if there is a previous byte
		 * (otherwise discard them silently).
		 */
		if(current_byte > 0)
			ctx->private_telegram_data[current_byte - 1] |= (
				(ctx->private_telegram_data[current_byte] & 3)
				<< 6);
		/*
		 * shift current byte two rightwards, making space for one
		 * datapoint (at the "highest" index)
		 */
		ctx->private_telegram_data[current_byte] >>= 2;
	}
}

static void dcf77_secondlayer_in_forward(struct dcf77_secondlayer* ctx)
{
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
			printf("    process_telegrams (1)\n"); /* DEBUG ONLY */
			dcf77_proc_process_telegrams(ctx);
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
			 * data arrangement, there should definitely
			 * have been a `NO_SIGNAL` at this place.
			 * We need to reorganize the datastructure
			 * to align to the "reality".
			 */
			printf("    recompute_eom because: NO_SIGNAL expected, leapexp=%d.\n", ctx->private_leap_second_expected);
			dcf77_proc_recompute_eom(ctx);
		}
	} else if(ctx->private_line_cursor == 60) {
		/*
		 * this is only allowed in the case of leap seconds.
		 * (no separate check here, but one could assert that
		 * the leap sec counter is > 0)
		 */
		if(ctx->in_val == DCF77_BIT_NO_SIGNAL) {
			/* ok, process this longer telegram */
			printf("    process_telegrams (2)\n");
			dcf77_proc_process_telegrams(ctx);
		} else {
			complex_reorganization(ctx);
		}
	} else {
		/*
		 * If we are not near the end, just quietly append
		 * received bits, i.e. advance cursor (the rest of
		 * that is already implemented above).
		 */
		ctx->private_line_cursor++;
	}
}

/*
 * This case is known as "complex reorganization"
 *
 * Condition: Would have expected an end-of-minute marker. We have a mismatch
 * although a leap-second was announced for around this time.
 *
 * Action: We rewrite the data to step back one position such that the case is
 * equivalent ot cursor = 59 and NO_SIGNAL expected. Then, we call
 * recompute_eom() to fix up the situation so far. Unlike in the case of the
 * actual cursor = 59 we still need to process that one misplaced bit. We do
 * this by repeating part of the computation.
 */
static void complex_reorganization(struct dcf77_secondlayer* ctx)
{
	/* clear the bit in memory under consideration */
	ctx->private_telegram_data[ctx->private_line_current *
					DCF77_SECONDLAYER_LINE_BYTES + 15] = 0;

	/* move the cursor */
	ctx->private_line_cursor--;

	/* call recompute_eom() */
	printf("    recompute_eom because NO_SINGAL expected complex reorganization!\n"); /* TODO DEBUG ONLY */
	dcf77_proc_recompute_eom(ctx);

	/*
	 * Process the misplaced bit.
	 * Does not decrease the leap second expectation again.
	 */
	dcf77_secondlayer_write_new_input(ctx);
	dcf77_secondlayer_automaton_case_specific_handling(ctx);
}
