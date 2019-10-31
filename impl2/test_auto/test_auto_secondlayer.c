#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_proc_xeliminate.h"
#include "dcf77_telegram.h"
#include "inc_sat.h"
#include "xeliminate_testcases.h"

/* TODO z once we have reorganization: do a test with a leap second (like test case 9) but "miss" the leap second in the sense that just a 3/NO_SIGNAL is output for the 0-marker in the leap second. It will be interesting to see how long it takes to recover from that! */

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
	 * For leap seconds, a regular telegram + length = 61 will be
	 * returned. This is as of now considered perfectly OK and stems
	 * from the xelimination not doing any elimination wrt. leapsec data.
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

static const unsigned char CMPMASK[16] = {
0x03,0x00,0x00,0x00,0x00,0x03,0x00,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff
/*
0xee,0xef,0xbb,0xbf,0xae,0xaf,0xbb,0xff,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a,
0x56,0x55,0x55,0x55,0x55,0x57,0x55,0xfd,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a,
0101 1101 1010 1110 0100 1100 1010 1111 0100 1100 1001 1001 0010 0110 0013 2
0333 3333 3333 3333 3333 1333 3333 3111 0100 1100 1001 1001 0010 0110 0013 2
*/
};

int main(int argc, char** argv)
{
	struct dcf77_high_level uut;

	unsigned char pass;
	unsigned char cmpbuf[DCF77_HIGH_LEVEL_LINE_BYTES];

	unsigned char curtest;
	unsigned char i;
	unsigned char j;
	unsigned char bitval;
	
	for(curtest = 0; curtest <
			(sizeof(xeliminate_testcases) /
			sizeof(struct xeliminate_testcase)); curtest++) {

		/* for now skip tests which fail recovery */
		if(!xeliminate_testcases[curtest].secondlayer_required &&
				xeliminate_testcases[curtest].recovery_ok == 0)
			continue;

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
			printf("  Line %2u -----------------------------------"
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

				/*
				printf("    > %d (%d)\n", bitval,
						xeliminate_testcases[curtest].
						data[i][j]);
				*/
				uut.in_val = bitval;
				dcf77_high_level_process(&uut);
				if(0 == 1) /* currently disabled */
					dumpmem(&uut);
				if(uut.out_telegram_1_len != 0) {
					printf("    out1len=%u out2len=%u\n",
							uut.out_telegram_1_len,
							uut.out_telegram_2_len);
					printtel(uut.out_telegram_1,
							uut.out_telegram_1_len);
					printtel(uut.out_telegram_2,
							uut.out_telegram_2_len);

					memcpy(cmpbuf,
						uut.out_telegram_2_len != 0?
							uut.out_telegram_2:
							uut.out_telegram_1,
						DCF77_HIGH_LEVEL_LINE_BYTES);

					uut.out_telegram_1_len = 0;
					uut.out_telegram_2_len = 0;
				}
			}
		}
		/* now compare */
		pass = 1;
		for(i = 0; i < DCF77_HIGH_LEVEL_LINE_BYTES; i++) {
			if((cmpbuf[i] & CMPMASK[i]) != (xeliminate_testcases[
					curtest].recovers_to[i] & CMPMASK[i])) {
				printf("  [FAIL] Mismatch at index %d\n", i);
				printtel(cmpbuf, 60);
				pass = 0;
				break;
			}
		}
		/*
		 * Note that this test is not very prcise, but if it fails, it
		 * is quite likely that there is something amiss.
		 */
		if(pass)
			printf("  [ OK ] Matches wrt. CMPMASK\n");
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
	printf("    [DEBUG] line_current=%u, cursor=%u\n",
			ctx->private_line_current, ctx->private_line_cursor);
}

/* ------------------------------------------------[ Logic Implementation ]-- */
static void reset(struct dcf77_high_level* ctx);
static void shift_existing_bits_to_the_left(struct dcf77_high_level* ctx);
static void process_telegrams(struct dcf77_high_level* ctx);
static inline unsigned char nextl(unsigned char inl);
static inline unsigned char prevl(unsigned char inl);
static void recompute_eom(struct dcf77_high_level* ctx);
static void advance_to_next_line(struct dcf77_high_level* ctx);
static void postprocess(struct dcf77_high_level* ctx,
		unsigned char* in_out_telegram, unsigned char* in_telegram);
static void add_missing_bits(unsigned char* in_out_telegram,
						unsigned char* in_telegram);
static void check_for_leapsec_announce(struct dcf77_high_level* ctx,
						unsigned char* telegram);

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
				/* TODO ASTAT N_IMPL / invoke recompute_eom(), afterwards the line will likely not be 100% full, but cursor reorganization is handled by compute_eom... / SUBSTAT: IT MIGHT MAKE SENSE TO IMPLEMENT TESTS WHICH DO NOT NEED THE REOGRANIZATION BY NOW AND THOROUGHLY TEST THAT THE EXISTING THINGS BEHAVE AS EXPECTED! */
			}
		} else if(ctx->private_line_cursor == 60) {
			/*
			 * this is only allowed in the case of leap seconds.
			 * (no separate check here, but one could assert that
			 * the leap sec counter is > 0)
			 */
			if(ctx->in_val == DCF77_BIT_NO_SIGNAL) {
				/* ok, process this longer telegram */
				process_telegrams(ctx);
			} else {
				/*
				 * would have expected an end-of-minute marker.
				 * We have a mismatch although a leap-second
				 * was announced for around this time.
				 *
				 * TODO Z RECOVER FROM THIS CASE BY DOING TWO STEPS: (1) move current bit to next line, then shift previous line one rightwards (this would actually need to go through the whole memory and fill the first space with a `2`/epsilon)? -- the problem here is: that is quite complicated. It would seem that it might be possible to integrate this functionality into the reorganization function, in any case it is not trivial and might be enough complexity to warrant a reset for this rare case of an announced leap second and async...
				 */
				printf("[DEBUG] COMPLEX REORGANIZATION REQUIRED\n");
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

	/* TODO z not sure if this -1 makes sense here / BAK for(current_byte = 15 - (ctx->private_line_lengths[0] / 4) - (ctx->private_line_lengths[0] % 4 != 0); */
	for(current_byte = (60 - ctx->private_line_lengths[0] - 1) / 4;
				current_byte < DCF77_HIGH_LEVEL_LINE_BYTES;
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
	/* TODO z printf("    [DEBUG] FROM %u to %u\n", nextl(ctx->private_line_current + 1), ctx->private_line_current); */
	for(match = 1, line = nextl(ctx->private_line_current);
				line != ctx->private_line_current && match;
				line = nextl(line)) {
		/* ignore empty lines */
		if(ctx->private_line_lengths[line] == 0)
			continue;

		/*
		printf("    [DEBUG] xeliminate1(\n");
		printf("    [DEBUG]   ");
		printtel_sub(ctx->private_telegram_data + 
					(DCF77_HIGH_LEVEL_LINE_BYTES * line));
		printf("    [DEBUG]   ");
		printtel_sub(ctx->out_telegram_1);
		*/
		match = dcf77_proc_xeliminate(ctx->private_line_lengths[line],
					lastlen, ctx->private_telegram_data +
					(DCF77_HIGH_LEVEL_LINE_BYTES * line),
					ctx->out_telegram_1);
		/*
		printf("    [DEBUG] )=%d\n", match);
		*/
		lastlen = ctx->private_line_lengths[line];
	}

	postprocess(ctx, ctx->out_telegram_1,
					ctx->private_telegram_data +
					(DCF77_HIGH_LEVEL_LINE_BYTES * line));

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
							line = nextl(line)) {
			/* ignore empty lines (relevant in the beginning) */
			if(ctx->private_line_lengths[line] == 0)
				continue;

			/*
			printf("    [DEBUG] xeliminate2(\n");
			printf("    [DEBUG]   ");
			printtel_sub(ctx->private_telegram_data + 
					(DCF77_HIGH_LEVEL_LINE_BYTES * line));
			printf("    [DEBUG]   ");
			printtel_sub(ctx->out_telegram_2);
			*/
			match = dcf77_proc_xeliminate(
					ctx->private_line_lengths[line],
					lastlen, ctx->private_telegram_data +
					(DCF77_HIGH_LEVEL_LINE_BYTES * line),
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
					(DCF77_HIGH_LEVEL_LINE_BYTES * line));
			advance_to_next_line(ctx);
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

/* includes clearing the next line's contents */
static void advance_to_next_line(struct dcf77_high_level* ctx)
{
	ctx->private_line_cursor = 0;
	ctx->private_line_current = nextl(ctx->private_line_current);
	ctx->private_line_lengths[ctx->private_line_current] = 0;
	memset(ctx->private_telegram_data +
		(ctx->private_line_current * DCF77_HIGH_LEVEL_LINE_BYTES),
		0, DCF77_HIGH_LEVEL_LINE_BYTES);
}

static inline unsigned char nextl(unsigned char inl)
{
	return (inl + 1) % DCF77_HIGH_LEVEL_LINES;
}

static inline unsigned char prevl(unsigned char inl)
{
	return ((inl == 0)? (DCF77_HIGH_LEVEL_LINES - 1): (inl - 1));
}

static void recompute_eom(struct dcf77_high_level* ctx)
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
	 * the presence of leap seconds?]
	 */

	/* TODO ASTAT N_IMPL */
	puts("ERROR,recompute_eom not implemented!");
	if(0 == 0)
		return;

	/* -- Step 1: Identify new eom position -- */

	/*
	 * again two bits per entry:
	 * 11 = no   mism good
	 * 10 = one  mism good if leap
	 * 01 = two+ mism never good
	 * 00 = (error)   too many decrements = bug = should not happen
	 */
	unsigned char current_valid_eom_options[DCF77_HIGH_LEVEL_LINE_BYTES];
	unsigned char line;
	unsigned char curbit;

	unsigned char curval;
	unsigned char curopt;

	/*
	 * Initially, all positions are potential new eoms...
	 * This version is not particulary optimized for performance but
	 * might do just OK.
	 */
	memset(current_valid_eom_options, 0xff,
					sizeof(current_valid_eom_options));

	for(line = nextl(ctx->private_line_current);
			line != ctx->private_line_current; line = nextl(line)) {
		/* skip empty lines */
		if(ctx->private_line_lengths[line] == 0)
			continue;

		for(curbit = 0; curbit < ctx->private_line_lengths[line];
								curbit++) {
			curopt = dcf77_telegram_read_bit(curbit,
						current_valid_eom_options);
			if(curopt == 1) /* no chance to use this, skip */
				continue;

			curval = dcf77_telegram_read_bit(
				curbit,
				ctx->private_telegram_data + (line *
						DCF77_HIGH_LEVEL_LINE_BYTES)
			);
			if(curval == DCF77_BIT_1)
				/* a place with 1 is never accepted */
				dcf77_telegram_write_bit(curbit,
					current_valid_eom_options, 1);
			else if(curval == DCF77_BIT_0)
				dcf77_telegram_write_bit(curbit,
					current_valid_eom_options, curopt - 1);
		}
	}

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
			if(curval == 2)
				break; /* OK, leap sec, this might be valid */
		}
	}
	if(curbit == 60) {
		/*
		 * nothing was found. this should normally be impossible.
		 * Do we have some bogus reading or attack. Or program
		 * bug (more likely). In any case, there is nothing but
		 * a reset to solve this data inconsistency
		 */
		reset(ctx);
		return;
	}

	/* -- Step 3: Apply new eom position -- */

	/*
	 * Everyone needs to move curbit steps to the left. This should honor
	 * the length of lines and consider the case of a reduction of the
	 * total number of lines.
	 * TODO CSTAT SUBSTAT IMPLEMENT THIS ROUTINE. THERE IS ALREADY ONE FOR A ONE BIT SHIFT, BUT WE NEED POTENTIALLY MULTIPLE BYTES AND THEN MULTIPLE BITS: MIGHT EVEN SEPARATE AFTER THIS LOGIC, BUT STILL THE SHIFT NEEDS TO CONCERN MULTIPLE BITS!
	 */
}

static void move_entries_backwards_old(struct dcf77_high_level* ctx,
							unsigned char mov)
{
	unsigned char mov_bytes   = mov / 4; /* how many bytes move backwards */
	unsigned char mov_entries = mov % 4; /* how many entries mov backward */

	unsigned char l0; /* line first   inclusive */
	unsigned char b0; /* byte first   inclusive */
	unsigned char bl; /* byte last    inclusive  */
	unsigned char bc; /* byte current */

	unsigned char shf;
	unsigned char upper_low;

	unsigned char dist = 0; /* distance from first byte processed */

	/*
	 * start from the first line in buffer.
	 * This is the first line following from the current which is not
	 * empty.
	 */
	for(l0 = nextl(ctx->private_line_current);
			ctx->private_line_lengths[l0] == 0 &&
			l0 != ctx->private_line_current; l0 = nextl(l0));
	
	b0 = l0 * DCF77_HIGH_LEVEL_LINE_BYTES;
	/* -1 for exclusive -> inclusive */
	bl = ctx->private_line_current * DCF77_HIGH_LEVEL_LINE_BYTES +
		ctx->private_line_lengths[ctx->private_line_current] - 1;

	/*
	 * Perform actual move.
	 *
	 * The idea is as follows: We split the current byte into two
	 * parts:
	 *
	 * - the "upper_low" part is the logically "higher/later" entries.
	 *   These will be moved less far (by mov_bytes) and be written
	 *   to the lower part of the target byte.
	 *   As for any byte, this step will happen "first", it is safe to
	 *   overwrite the data here (instead of or-ing), saving an explicit
	 *   delete step.
	 * - The "lower up" part is the logically "lower/earlier" entries.
	 *   These will be moved farther (by mov_bytes - 1) and be written
	 *   to the upper part of the target byte. As this byte has already
	 *   been "set" by the previous step in the previous iteration,
	 *   it is required to use "or" here.
	 *
	 * TODO CSTAT ALTHOUGH THE IDEA IS NICE, THERE IS STILL A MAJOR PROBLEM HERE: HOW DO WE COVER LINE LENGHTS HERE. THRE PROBLEM IS THAT LINES ARE NOT "FULL" (60 or 61 instead of the theoretically possible 64 entries). We thus need to move in a way that the "ends" of these bytes are not moved but rather "ignored" of sorts... Normalerweise werden nur die indices 0--14 benötigt (bits 0..59). Wenn ein bit60 vorhanden ist, dann ist es eine Schalteskunde. Man kann sich das in der Theorie so vorstellen, dass einfach nur die Bytes [0..14] bearbeitet werden und dann als "Sondersache" noch der eine Eintrag von der Schaltsekunde mitgenommen wird, falls er existiert. Weiterhin bedeutet das Verarbeiten eines solchen Eintrages generell, dass nachfoglgend eine "Verschiebung in der Verschiebung" vorliegt -- die relative Verschiebung der Einträge wird nämlich um 1 reduziert!
	 */
	for(bc = b0; bc != bl; bc = ((bc + 1) % (DCF77_HIGH_LEVEL_LINES *
						DCF77_HIGH_LEVEL_LINE_BYTES))) {
		upper_low = ctx->private_telegram_data[bc] >> (2 * mov_entries);
		shf = 8 - 2 * mov_entries;
		if(mov_bytes + 1 <= dist)
			ctx->private_telegram_data[bc - mov_bytes - 1] |=
				/* lower up */
				((0xff >> shf) &
				ctx->private_telegram_data[bc]) << shf;
		if(mov_bytes <= dist)
			ctx->private_telegram_data[bc - mov_bytes] = upper_low;
		dist++;
	}
}

#define MOVENTRIES_ADVANCE_OUTPUT_CURSOR \
	{ \
		if(pol == 14) { \
			pol = 0; \
			if(next_produce_leapsec_marker) { \
				next_produce_leapsec_marker   = 0; \
				ctx->private_line_lengths[ol] = 61; \
			} else { \
				ctx->private_line_lengths[ol] = 60; \
			} \
			prevoll = ctx->private_line_lengths[ol]; \
			ol = nextl(ol); \
		} else { \
			pol++; \
		} \
	}

/* @param mov: aka. v */
static void move_entries_backwards(struct dcf77_high_level* ctx,
		unsigned char mov, unsigned char in_line_holding_leapsec_marker)
{
	unsigned char il0; /* line in first inclusive */

	unsigned char previll; /* previous input line length */
	unsigned char il;      /* current input line */
	unsigned char ill;     /* current input line length */
	unsigned char pil;     /* position in input line */
	unsigned char bytes_to_proc;

	unsigned char readib;  /* read input byte position in memory */
	/*
	 * upper bytes will be moved "forward" and placed at a low position
	 * within the byte
	 */
	unsigned char upper_low;
	/*
	 * lower bytes will be moved "backward" and placed at a high position
	 * within the byte
	 */
	unsigned char lower_up;

	unsigned char next_produce_leapsec_marker; /* bool */
	unsigned char produce_leap; /* bool */
	unsigned char send_back_offset;
	unsigned char ol;
	unsigned char wrpos;

	unsigned char bytes_proc = 0;

	char prevoll = -1;     /* previous output line length */
	unsigned char pol = 0; /* position output line */

	unsigned char mov_entries = (mov % 4);
	unsigned char mov_bytes_initial = mov / 4;

	/* -- Bestimme l0 -- */

	/*
	 * start from the first line in buffer.
	 * This is the first line following from the current which is not
	 * empty.
	 */
	for(il0 = nextl(ctx->private_line_current);
			ctx->private_line_lengths[il0] == 0 &&
			il0 != ctx->private_line_current; il0 = nextl(il0));

	ol = il0 * DCF77_HIGH_LEVEL_LINE_BYTES;

	/* -- Hauptprozedur -- */
	next_produce_leapsec_marker = 0;
	il = il0;
	do {
		next_produce_leapsec_marker |=
					(il == in_line_holding_leapsec_marker);
		ill = ctx->private_line_lengths[il];
		/* ctx->private_line_lengths[il] = 0; * superflous? */

		/* skip empty input lines */
		if(ill == 0)
			continue;

		/* 
		 * 0..14 werden immer verarbeitet
		 * 15    nur bei leapsec und dann von einer separaten Logik
		 * ob man <60 oder <61 schreibt ist egal.
		 */
		bytes_to_proc = ill / 4 + ((ill % 4) != 0 && ill < 60);
		for(pil = 0; pil < bytes_to_proc; pil++) {
			/* -- Verarbeiteprozedur: Lies Eingabe -- */
			readib = pil + il * DCF77_HIGH_LEVEL_LINE_BYTES;
			upper_low = ctx->data[readib] >> (2 * mov_entries);
			shf = 8 * 2 - mov_entries;
			lower_up = ((0xff >> shf) & ctx->data[readib]) << shf;
			if(pil == 0 && previll == 61) {	
				/* leap_in = X (might also want to read) */
				upper_low = (upper_low << 2) |
							DCF77_BIT_NO_SIGNAL;
				mov--;
				mov_entries = mov % 4;
				if(mov_entries == 0)
					MOVENTRIES_ADVANCE_OUTPUT_CURSOR
			}
			/* -- Verarbeiteprozedur: Schreibe Ausgabe -- */
			produce_leap = ((prevoll == 61) && (pol == 0));
			send_back_offset = (produceleap || pol == 0)? 2: 1;
			wrpos = pol + ol * DCF77_HIGH_LEVEL_LINE_BYTES;
			if(bytes_proc >= (mov_bytes_initial + send_back_offset))
				ctx->data[wrpos - send_back_offset] |= lower_up;

			if(produce_leap) {
				if(bytes_proc >= (mov_bytes_initial + 1))
					ctx->data[wrpos - 1] =
							DCF77_BIT_NO_SIGNAL;
				upper_low <<= 2; /* cancel lowermost entry */
			}
			ctx->data[wrpos] |= upper_low;
			INC_SAT(bytes_proc);
			if(produce_leap) {
				mov++;
				mov_bytes = mov % 4;
				if(mov_bytes == 0)
					continue; /* skip addr inc */
			}
			MOVENTRIES_ADVANCE_OUTPUT_CURSOR
		}
		previll = ill;
	} while((il = nextl(il)) != l0);
	
	/* -- Abschließende Aktualisierungen -- */
	/* TODO SET LAT LINE LENGTH, THEN SET CURSOR; ctx->private_line_lengths[ol] = ... */
}

static void postprocess(struct dcf77_high_level* ctx,
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

static void check_for_leapsec_announce(struct dcf77_high_level* ctx,
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
