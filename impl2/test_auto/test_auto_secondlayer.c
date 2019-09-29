#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_proc_xeliminate.h"
#include "inc_sat.h"
#include "xeliminate_testcases.h"

/* TODO CSTAT SUBSTAT BETRACHTE LETZES BILD AUS TEST: FALSCH GESCHRIEBEN UND FALSCH ELIMINIERT? */

/* ---------------------------------------[ Logic Declaration / Interface ]-- */

#define DCF77_HIGH_LEVEL_LINES       9
#define DCF77_HIGH_LEVEL_TIME_LEN    8
#define DCF77_HIGH_LEVEL_DATE_LEN   10
#define DCF77_HIGH_LEVEL_LINE_BYTES 16
#define DCF77_HIGH_LEVEL_MEM        (DCF77_HIGH_LEVEL_LINE_BYTES * \
							DCF77_HIGH_LEVEL_LINES)

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

	unsigned char fault_reset; /* Number of resets performed */

	/* input */
	enum dcf77_bitlayer_reading in_val;

	/*
	 * output
	 *
	 * Logic is a s follows:
	 * if only out_telegram_1_len is > 0, then process that as truth
	 * if both out_telegram_1_len and telegram_2_len are > 0 then
	 * 	process telegram_2 as truth and telegram_1 is from the
	 * 	previous 10min interval.
	 * if both are <0, then no new telegram data exists.
	 *
	 * implicit information
	 *
	 * has new second <=>
	 * 	has new measurement
	 * 	(lower layer information may bypass second layer here)
	 * has new telegram <=> out_telegram_1_len != 0
	 *
	 * users should reset out_telegram_len values after processing!
	 */
	unsigned char out_telegram_1_len; /* in bits */
	unsigned char out_telegram_2_len; /* in bits */
	unsigned char out_telegram_1[DCF77_HIGH_LEVEL_LINE_BYTES];
	unsigned char out_telegram_2[DCF77_HIGH_LEVEL_LINE_BYTES];
};

void dcf77_high_level_init(struct dcf77_high_level* ctx);
void dcf77_high_level_process(struct dcf77_high_level* ctx);

/* -------------------------------------------------[ Test Implementation ]-- */

static void printtel(unsigned char* data, unsigned char bitlen);
static void printtel_sub(unsigned char* data);
static void dumpmem(struct dcf77_high_level* ctx);

int main(int argc, char** argv)
{
	struct dcf77_high_level uut;

	unsigned char curtest;
	unsigned char i;
	unsigned char j;
	unsigned char bitval;
	
	for(curtest = 0; curtest <
			(sizeof(xeliminate_testcases) /
			sizeof(struct xeliminate_testcase)); curtest++) {

		puts("======================================================="
						"=========================");
		printf("Test case %u: %s\n", curtest,
				xeliminate_testcases[curtest].description);
		/*
		 * we initialize everything to 0 to avoid data from the previous
		 * tests to be present. disable this once the test runs
		 * automatically TODO z
		 */
		memset(&uut, 0, sizeof(struct dcf77_high_level));
		dcf77_high_level_init(&uut);

		for(i = 0; i < xeliminate_testcases[curtest].num_lines; i++) {
			printf("  Line %u ------------------------------------"
				"-----------------------------------\n", i);
			for(j = 0; j <
			xeliminate_testcases[curtest].line_len[i]; j++) {
				switch(xeliminate_testcases[curtest].
								data[i][j]) {
				case 0:  bitval = DCF77_BIT_0;         break;
				case 1:  bitval = DCF77_BIT_1;         break;
				case 2:  bitval = DCF77_BIT_NO_UPDATE; break;
				case 3:  bitval = DCF77_BIT_NO_SIGNAL; break;
				default: puts("    *** ERROR1 ***"); exit(64);
				}

				printf("    > %d (%d)\n", bitval,
						xeliminate_testcases[curtest].
						data[i][j]);
				uut.in_val = bitval;
				dcf77_high_level_process(&uut);
				dumpmem(&uut);
				if(uut.out_telegram_1_len != 0) {
					printf("    out1len=%u out2len=%u\n",
							uut.out_telegram_1_len,
							uut.out_telegram_2_len);
					printtel(uut.out_telegram_1,
							uut.out_telegram_1_len);
					printtel(uut.out_telegram_2,
							uut.out_telegram_2_len);
					uut.out_telegram_1_len = 0;
					uut.out_telegram_2_len = 0;
				}
			}
		}
		break; /* TODO CSTAT DEBUG ONLY. AS OF NOW IT FAILS CATASTROPHICALLY WITH THE FIRST TEST ALREADY! */
	}
}

static void printtel(unsigned char* data, unsigned char bitlen)
{
	printf("    tellen=%2u val=", bitlen);
	printtel_sub(data);
}

static void printtel_sub(unsigned char* data)
{
	unsigned char j;
	for(j = 0; j < 15; j++)
		printf("%02x,", data[j]);
	putchar('\n');
}

static void dumpmem(struct dcf77_high_level* ctx)
{
	unsigned char i;
	printf("    [DEBUG]         ");
	for(i = 0; i < DCF77_HIGH_LEVEL_LINE_BYTES; i++) {
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
	for(i = 0; i < DCF77_HIGH_LEVEL_LINES; i++) {
		printf("    [DEBUG] %s meml%d=", i == ctx->private_line_current?
								"*": " ", i);
		printtel_sub(ctx->private_telegram_data +
					(i * DCF77_HIGH_LEVEL_LINE_BYTES));
	}
}

/* ------------------------------------------------[ Logic Implementation ]-- */
static void reset(struct dcf77_high_level* ctx);
static void shift_existing_bits_to_the_left(struct dcf77_high_level* ctx);
static void process_telegrams(struct dcf77_high_level* ctx);
static inline unsigned char nextl(unsigned char inl);
static void recompute_eom(struct dcf77_high_level* ctx);

void dcf77_high_level_init(struct dcf77_high_level* ctx)
{
	reset(ctx);
	ctx->fault_reset = 0; /* reset number of resets "the first is free" */
}

static void reset(struct dcf77_high_level* ctx)
{
	ctx->private_inmode               = IN_BACKWARD;
	ctx->private_line_current         = 0;
	ctx->private_line_cursor          = 59;
	ctx->private_leap_second_expected = 0; /* no leap second expected */
	/* denote number of resets */
	INC_SATURATED(ctx->fault_reset);
	/* initialize with 0 */
	memset(ctx->private_line_lengths,  0, DCF77_HIGH_LEVEL_LINES);
	/* initialize with epsilon */
	memset(ctx->private_telegram_data, 0, DCF77_HIGH_LEVEL_MEM);

	ctx->out_telegram_1_len = 0;
	ctx->out_telegram_2_len = 0;
}

void dcf77_high_level_process(struct dcf77_high_level* ctx)
{
	/* do nothing if no update */
	if(ctx->in_val == DCF77_BIT_NO_UPDATE)
		return;

	/* write new input */
	ctx->private_telegram_data[
			ctx->private_line_current * DCF77_HIGH_LEVEL_LINE_BYTES
			+ ctx->private_line_cursor / 4] |= 
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
			/*
			 * we set the length to 60 because now the epsilons
			 * become part of the telegram.
			 */
			ctx->private_line_lengths[0] = 60;
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
			reset(ctx);
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
				process_telegrams(ctx);
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
				recompute_eom(ctx);
				/* TODO N_IMPL / invoke recompute_eom(), afterwards the line will likely not be 100% full, but cursor reorganization is handled by compute_eom... / SUBSTAT: IT MIGHT MAKE SENSE TO IMPLEMENT TESTS WHICH DO NOT NEED THE REOGRANIZATION BY NOW AND THOROUGHLY TEST THAT THE EXISTING THINGS BEHAVE AS EXPECTED! */
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

static void process_telegrams(struct dcf77_high_level* ctx)
{
	unsigned char lastlen = 60;
	unsigned char line;
	char match;

	/* Input situation: cursor is at the end of the current minute. */

	/* first clear buffers to no signal */
	memset(ctx->out_telegram_1, 0x55, DCF77_HIGH_LEVEL_LINE_BYTES);
	memset(ctx->out_telegram_2, 0x55, DCF77_HIGH_LEVEL_LINE_BYTES);

	/* merge till mismatch */
	for(match = 1, line = nextl(ctx->private_line_current + 1);
				line != ctx->private_line_current && match;
				line = nextl(line)) {
		/* ignore empty lines */
		if(ctx->private_line_lengths[line] == 0)
			continue;

		match = dcf77_proc_xeliminate(ctx->private_line_lengths[line],
					lastlen, ctx->private_telegram_data +
					(DCF77_HIGH_LEVEL_LINE_BYTES * line),
					ctx->out_telegram_1);
		printf("    [DEBUG] xeliminate1=%d\n", match);
		lastlen = ctx->private_line_lengths[line];
	}

	if(match) {
		/* that was already the actual output */
		ctx->out_telegram_1_len   = lastlen;
		ctx->out_telegram_2_len   = 0;
		ctx->private_line_current = nextl(ctx->private_line_current);
		/* return */
	} else {
		/* repeat and write to actual output */
		ctx->out_telegram_1_len = lastlen;

		match = 1;
		for(; line != ctx->private_line_current && match;
							line = nextl(line)) {
			/* ignore empty lines */
			if(ctx->private_line_lengths[line] == 0)
				continue;

			match = dcf77_proc_xeliminate(
					ctx->private_line_lengths[line],
					lastlen, ctx->private_telegram_data +
					(DCF77_HIGH_LEVEL_LINE_BYTES * line),
					ctx->out_telegram_2);
			printf("    [DEBUG] xeliminate2=%d\n", match);
			lastlen = ctx->private_line_lengths[line];
		}

		if(match) {
			/*
			 * no further mismatch. Data in the buffer is fully
			 * consistent. Can output this as truth
			 */
			ctx->out_telegram_2_len = lastlen;
			ctx->private_line_current =
					nextl(ctx->private_line_current);
			/* return */
		} else {
			/*
			 * we got another mismatch. this means the data is
			 * not consistent.
			 */
			ctx->out_telegram_1_len = 0;
			ctx->out_telegram_2_len = 0;
			recompute_eom(ctx);
			/* TODO CALL recompute_eom(), then re-invoke as described on paper. Remember that this has to advance line... */
		}
	}
}

static inline unsigned char nextl(unsigned char inl)
{
	return (inl + 1) % DCF77_HIGH_LEVEL_LINES;
}

static void recompute_eom(struct dcf77_high_level* ctx)
{
	/* TODO N_IMPL */
	puts("ERROR,recompute_eom not implemented!");
}
