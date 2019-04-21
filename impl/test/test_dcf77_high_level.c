#include <stdio.h>
#include <string.h>

#include "dcf77_low_level.h"

/* definition here is debug only */
static char xeliminate(size_t telegram_1_len, size_t telegram_2_len,
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2);

int main(int argc, char** argv)
{
	unsigned char i;
	unsigned char j;
	unsigned char example_data[3][60] = {
		/* 13.04.19 */
		{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3}, /* 17:28 */
        /*       0 1 2 3 4 5 6 7 8 9101112131415161718 */
		{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,0,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3}, /* 17:29 */
		/* Verrauschtes Testtelegram funktioniert auch: */
		/*{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,0,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,3,3,3,3,3,3,3,3,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3}, * 17:29 */
		{0,1,0,0,0,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,1,0,0,0,0,1,1,0,0,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,3,3,3,3,3,3,3,3}, /* 17:30 */
	};
	unsigned char telegram_1[15];
	unsigned char telegram_2[15];
	unsigned char* telegram[] = { telegram_1, telegram_2 };
	unsigned char bitval;
	memset(telegram_1, 0, sizeof(telegram_1) / sizeof(unsigned char));
	memset(telegram_2, 0, sizeof(telegram_1) / sizeof(unsigned char));
	for(i = 0; i < 2; i++) {
		for(j = 0; j < 60; j++) {
			switch(example_data[i][j]) {
			case 0:  bitval = 2; break;
			case 1:  bitval = 3; break;
			case 2:  bitval = 0; break;
			case 3:  bitval = 1; break;
			default: puts("<<<ERROR1>>>"); return 64;
			}
			telegram[i][j / 4] |= bitval << ((j % 4) * 2);
		}
		printf("Telegram %d:    ", i);
		for(j = 0; j < sizeof(telegram_1)/sizeof(unsigned char); j++)
			printf("%02x,", telegram[i][j]);
		puts("");
	}
	i = xeliminate(60, 60, telegram_1, telegram_2);
	printf("Eliminated [%d] ", i);
	for(j = 0; j < sizeof(telegram_2)/sizeof(unsigned char); j++)
		printf("%02x,", telegram_2[j]);
	puts("");
	return 0;
}

/* interface */
#define DCF77_HIGH_LEVEL_MEM   138
#define DCF77_HIGH_LEVEL_LINES 9
#define DCF77_HIGH_LEVEL_TIME_LEN 8
#define DCF77_HIGH_LEVEL_DATE_LEN 10

enum dcf77_high_level_input_mode { IN_INIT, IN_ALIGNED, IN_UNKNOWN };

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
#define VAL_EPSILON 0 /* 00 */
#define VAL_X       1 /* 01 */
#define VAL_0       2 /* 10 */
#define VAL_1       3 /* 11 */

static char xeliminate_entry(unsigned char in_1, unsigned char* in_out_2,
							unsigned char entry);
static unsigned char read_entry(unsigned char in, unsigned char entry);

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

/* ----------------------------------------[ Procedures for X-elimination ]-- */
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
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2)
{
	unsigned char i;
	unsigned char etmp;
	unsigned char etmp2;
	unsigned char* telleap;
	unsigned char* telregular;

	/* 0:    entry has to match and be constant 0 */
	if(!xeliminate_entry(*in_telegram_1, in_out_telegram_2, 0)) {
		puts("<<<ERROR2>>>");
		return 0;
	}

	etmp = read_entry(*in_out_telegram_2, 0);
	if(etmp == VAL_1) {
		puts("<<<ERROR3>>>");
		return 0; /* constant 0 violated */
	}

	/* 16--20: entries have to match */
	for(i = 16; i <= 20; i++) {
		if(!xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4)) {
			puts("<<<ERROR4>>>");
			return 0;
		}
	}

	/* 17+18: needs to be 10 or 01 */
	etmp  = read_entry(in_out_telegram_2[4], 1);
	etmp2 = read_entry(in_out_telegram_2[4], 2);
	/* assertion violated if 00 or 11 found */
	if((etmp == VAL_0 && etmp2 == VAL_0) ||
					(etmp == VAL_1 && etmp2 == VAL_1)) {
		printf("<<<ERROR5,etmp1=%u,etmp2=%u>>>\n", etmp, etmp2);
		return 0;
	}
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
	if(etmp == VAL_0) {
		puts("<<<ERROR6>>>");
		return 0; /* constant 1 violated */
	}

	/* 25--58: entries have to match */
	for(i = 25; i <= 58; i++) {
		if(!xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4)) {
			printf("<<<ERROR7,i=%u>>>\n", i);
			return 0;
		}
	}

	/* 59:   entries have to match and be constant X
	 *       (or special case leap second) */
	if(telegram_1_len == 60 && telegram_2_len == 60) {
		/* needs to be X */
		return (read_entry(in_telegram_1[14], 3) == VAL_X &&
				read_entry(in_out_telegram_2[14], 3) == VAL_X);
	} else if((telegram_1_len == 61 && telegram_2_len == 60) ||
			(telegram_1_len == 60 && telegram_2_len == 61)) {
		/* Check the larger one */
		if(telegram_1_len == 61) {
			telleap    = in_telegram_1;
			telregular = in_out_telegram_2;
		} else {
			telleap    = in_out_telegram_2;
			telregular = in_telegram_1;
		}
		/*
		 * Now the check to perform is that the added part
		 * just before the end marker needs to be a `0` (X is
		 * allowed as well)
		 *
		 * Leap seconds are rare. We do not use them to
		 * correct data in the telegram (does not make sense
		 * to write to telegram_1 which is an in-variable
		 * anyways).
		 */
		return (read_entry(telleap[14],    3) == VAL_X  ||
			read_entry(telleap[14],    3) == VAL_0) &&
			read_entry(telleap[14],    4) == VAL_X  &&
			read_entry(telregular[14], 3) == VAL_X;
	} else {
		/* not a minute's telegram -> invalid */
		return 0;
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
	return (in & (3 << (entry * 2))) >> (entry * 2);
}
