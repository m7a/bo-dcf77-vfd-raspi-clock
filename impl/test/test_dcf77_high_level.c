#include <stdio.h>

#include "dcf77_low_level.h"

int main(int argc, char** argv)
{

}

/* interface */
#define DCF77_HIGH_LEVEL_MEM   138
#define DCF77_HIGH_LEVEL_LINES 9
#define DCF77_HIGH_LEVEL_TIME_LEN 8
#define DCF77_HIGH_LEVEL_DATE_LEN 10

enum dcf77_high_level_input_mode;

struct dcf77_high_level {
	/* private */
	enum dcf77_high_level_input_mode private_inmode;
	unsigned char private_telegram_data[DCF77_HIGH_LEVEL_MEM];
	unsigned char private_line_starts[DCF77_HIGH_LEVEL_LINES];
	unsigned char private_line_ends[DCF77_HIGH_LEVEL_LINES];
	unsigned char private_line_current;
	unsigned char private_line_cursor;
	/* input */
	enum dcf77_low_level_reading in_val;
	/* output */
	char out_time[DCF77_HIGH_LEVEL_TIME_LEN + 1]; /* TODO z requires copying to screen. Should avoid that by using just a pointer for one of them... Datetimelen is constant. */
	char out_date[DCF77_HIGH_LEVEL_DATE_LEN + 1];
};

void dcf77_high_level_init(struct dcf77_high_level* ctx);
void dcf77_high_level_process(struct dcf77_high_level* ctx);

/* internal */
enum dcf77_high_level_input_mode { IN_INIT, IN_ALIGNED, IN_UNKNOWN };

#define VAL_EPSILON 0 /* 00 */
#define VAL_X       1 /* 01 */
#define VAL_0       2 /* 10 */
#define VAL_1       3 /* 11 */

/* implementation */
void dcf77_high_level_init(struct dcf77_high_level* ctx)
{
	ctx->private_inmode = IN_INIT;
	ctx->private_line_starts[0] = 0;
	ctx->private_line_current = 0;
	ctx->private_line_cursor = 0;
	ctx->out_time[0] = 0;
	ctx->out_date[0] = 0;
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

/* -----------------------------------------[ Procedures for X-elimination ]-- */
/*
 * The purpose of this part is to implement a logic for merging information
 * from adjacent minutes as to reconstruct noisy telegrams. This works by
 * filling X-parts with 0/1 from the other message if applicable.
 */

/*
 * reads from 1 and 2 and writes to 2.
 *
 * @param telegram_len number of entries in telegram (60 or 61)
 * @return 0 if mismatch, 1 if OK
 */
static char xeliminate(size_t telegram_1_len, size_t telegram_2_len,
		unsigned char* in_telgram_1, unsigned char* in_out_telegram_2)
{
	unsigned char i;
	unsigned char etmp;
	unsigned char etmp2;

	/* 0:    entry has to match and be constant 0 */
	if(!xeliminate_entry(*in_telegram_1, in_out_telegram_2, 0))
		return 0;

	etmp = read_entry(*in_out_telegram_2, 0);
	if(etmp == VAL_1)
		return 0; /* constant 0 violated */

	/* 16--20: entries have to match */
	for(i = 16; i <= 20; i++)
		if(!xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4))
			return 0;

	/* 17+18: needs to be 10 or 01 */
	etmp  = read_entry(in_out_telegram_2[4], 1);
	etmp2 = read_entry(in_out_telegram_2[4], 2);
	/* assertion violated if 00 or 11 found */
	if((etmp == VAL_0 && etmp2 == VAL_0) ||
					(etmp == VAL_1 && etmp2 == VAL_1))
		return 0;
	if(etmp2 == VAL_X && etmp != VAL_X && etmp != VAL_EPSILON) {
		/*
		 * use 17 to infer value of 18
		 *
		 * Write inverse value of etmp (0->1, 1->0) to entry 2 in
		 * byte 4 (=18)
		 */
		in_out_telegram_2[4] =
			(in_out_telegram_2[4] & 0xcf) | ((etmp ^ 1) << 4);
	} else if(etmp == VAL_X && etmp2 != VAL_X && etmp2 != VAL_EPSILON) {
		/* use 18 to infer value of 17 */
		in_out_telegram_2[4] =
			(in_out_telegram_2[4] & 0xf3) | ((etmp2 ^ 1) << 2);
	}

	/* 20:     entry has to match and be constant 1 */
	etmp = read_entry(in_out_telegram_2[5], 0);
	if(etmp == VAL_0)
		return 0; /* constant 1 violated */

	/* 25--58: entries have to match */
	for(i = 25; i <= 58; i++)
		if(!xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4))
			return 0;

	/* 59:   entries have to match and be constant X
	 *       (or special case leap second) */
	if(telegram_1_len == telegram_2_len) {
		/* needs to be X */
		return (read_entry(in_telegram_1[12], 0) == VAL_X &&
				read_entry(in_out_telegram_2[12], 0) == VAL_X);
	} else {
		/* leap second case TODO N_IMPL THEN FOLLOWUP WRITE SOME TEST CASES FOR XELIMINATE FUNCTION (AND XELIMINATE ENTRY, READ_ENTRY ETC?) */
	}
} 

static char xeliminate_entry(unsigned char in_1, unsigned char* in_out_2,
							unsigned char entry)
{
	unsigned char val1 = read_entry(in_1,      entry);
	unsigned char val2 = read_entry(*in_out_2, entry);

	if(val1 == VAL_X || val1 == VAL_EPSILON || val1 == val2) {
		/* no update */
		return 1; /* OK */
	} else if(val2 == VAL_X || val2 == VAL_EPSILON) {
		/* takes val 1 */
		*in_out_2 = (*in_out_2 & ~(3 << (entry * 2))) |
							(val1 << (entry * 2));
		return 1;
	}

	/* mismatch */
	return 0;
}

static unsigned char read_entry(unsigned char in, unsigned char entry)
{
	return in & (3 << (entry * 2)) >> (entry * 2);
}
