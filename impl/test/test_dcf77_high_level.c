#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_low_level.h"

static void run_xeliminate_testcases();

int main(int argc, char** argv)
{
	run_xeliminate_testcases();
}

/* -----------------------------------------------------[ Test XELIMINATE ]-- */

/* definition here is debug only */
static char xeliminate(size_t telegram_1_len, size_t telegram_2_len,
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2);

struct xeliminate_testcase {
	char* description;
	unsigned char num_lines;
	unsigned char line_len[9];
	unsigned char data[9][61];
	unsigned char recovery_ok;
	unsigned char recovers_to[16];
};

struct xeliminate_testcase xeliminate_testcases[] = {
	{
		.description = "17:28 -> 17:29 eliminates correctly",
		.num_lines = 2,
		.line_len = { 60, 60 },
		.data = {
			/* 13.04.19 17:28 */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
			/* 17:29 */
			{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,0,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
		},
		.recovery_ok = 1,
		.recovers_to = {0xee,0xef,0xbb,0xbf,0xae,0xaf,0xbb,0xff,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a},
	},
	{
		.description = "17:29 -> 17:30 fails to eliminate",
		.num_lines = 2,
		.line_len = { 60, 60 },
		.data = {
			/* 13.04.19 17:29 */
			{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,0,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
			/* 17:30 */
			{0,1,0,0,0,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,1,0,0,0,0,1,1,0,0,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,3,3,3,3,3,3,3,3},
		},
		.recovery_ok = 0,
	},
	{
		.description = "17:28 -> 17:29 recover noisy telegram",
		.num_lines = 2,
		.line_len = { 60, 60 },
		.data = {
			/* 13.04.19 17:28 */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
			/* 17:29 */
			{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,0,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,3,3,3,3,3,3,3,3,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
		},
		.recovery_ok = 1,
		.recovers_to = {0xee,0xef,0xbb,0xbf,0xae,0xaf,0xbb,0xff,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a},
	},
	{
		.description = "17:28 -> 17:29 recover summertime bit 17 from 18",
		.num_lines = 2,
		.line_len = { 60, 60 },
		.data = {
			/* 13.04.19 17:28 */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
			/* 17:29 bit 17 defunct */
			{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,3,0,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
		},
		.recovery_ok = 1,
		.recovers_to = {0xee,0xef,0xbb,0xbf,0xae,0xaf,0xbb,0xff,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a},
	},
	{
		.description = "17:28 -> 17:29 recover summertime bit 18 from 17",
		.num_lines = 2,
		.line_len = { 60, 60 },
		.data = {
			/* 13.04.19 17:28 */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
			/* 17:29 bit 18 defunct */
			{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,3,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
		},
		.recovery_ok = 1,
		.recovers_to = {0xee,0xef,0xbb,0xbf,0xae,0xaf,0xbb,0xff,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a},
	},
	{
		.description = "17:28 -> 17:29 summertime recovery fails if both defunct",
		.num_lines = 2,
		.line_len = { 60, 60 },
		.data = {
			/* 13.04.19 17:28 */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
			/* 17:29 bits 17+18 defunct */
			{0,1,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,3,3,0,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3},
		},
		.recovery_ok = 1,
		.recovers_to = {0xee,0xef,0xbb,0xbf,0x96,0xaf,0xbb,0xff,0xae,0xaf,0xeb,0xeb,0xba,0xbe,0x7a},
	},
	/*
		https://www.dcf77logs.de/live  22.04.2019
		22:50 {0,1,1,0,1,1,0,1,1,1,0,1,1,1,1,0,0,1,0,0,1,0,0,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,}
		22:51 {0,1,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,1,0,0,1,1,0,0,0,1,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,}
		22:52 {0,0,0,1,1,0,1,1,0,1,1,0,0,0,1,0,0,1,0,0,1,0,1,0,0,1,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,}
		22:53 {0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,1,1,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,}
	*/
	{
		.description = "22:41 -> 22:49 complete correct entries recover to latest entry",
		.num_lines = 9,
		.line_len = { 60, 60, 60,  60, 60, 60,  60, 60, 60 },
		.data = {
			/* 22.04.19 22:41 */
			{0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0,0,1,0,0,1,1,0,0,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,1,0,1,1,1,0,1,1,1,0,0,0,1,0,0,0,1,0,0,1,0,1,0,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,0,1,1,0,0,0,0,0,0,0,1,1,0,1,0,0,1,0,0,1,1,1,0,0,0,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,0,1,0,0,0,1,1,0,0,0,0,0,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,0,0,1,0,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,1,0,1,0,0,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,0,1,0,1,0,1,1,0,0,1,0,1,1,1,0,0,1,0,0,1,0,1,1,0,0,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,0,0,1,0,0,1,1,1,1,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			{0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
			/* 22.04.19 22:49 */
			{0,0,1,1,1,0,0,1,0,1,0,1,0,1,0,0,0,1,0,0,1,1,0,0,1,0,0,1,1,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,1,3,},
		},
		.recovery_ok = 1,
		.recovers_to = {0xfa,0xeb,0xee,0xae,0xae,0xaf,0xeb,0xbb,0xba,0xae,0xbe,0xea,0xba,0xbe,0x7a},
	},
	{
		.description = "22:41 -> 22:49 noisy entries recover to full entry",
		.num_lines = 9,
		.line_len = { 60, 60, 60,  60, 60, 60,  60, 60, 60 },
		.data = {
			/*
			 * 22.04.19 22:41 all weather information deleted.
			 *
			 * also each message has only six bits which still
			 * allows for the recovery of datetime information
			 * over the course of ten minutes
			 */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,1,0,0,3,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,0,0,0,0,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,0,1,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,0,0,1,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,1,1,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,1,0,0,1,0,3,3,3,3,3,3,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,1,1,0,0,0,3,3,},
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,3,},
			/* 22.04.19 22:49 highly distorted (everything lost except for minute ones bits) */
			{3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,0,0,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,},
		},
		.recovery_ok = 1,
		.recovers_to = {0x56,0x55,0x55,0x55,0xae,0xaf,0xeb,0xb9,0xba,0xae,0xbe,0xea,0xba,0xbe,0x7a},
	},
	{
		.description = "02:00 -> 02:02 leap second",
		.num_lines = 3,
		.line_len = { 61, 60, 60, },
		.data = {
			/* 01.07.12 02:00 w/ leap second */
			{0,0,0,0,1,1,0,1,1,1,1,1,1,0,1,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,1,0,0,1,0,0,0,1,0,3,},
			/* 02:01 */
			{0,0,1,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,0,0,1,1,0,0,0,0,0,0,1,0,1,0,0,0,0,1,1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,1,0,0,1,0,0,0,1,3,},
			/* 02:02 */
			{0,0,1,0,0,1,1,1,0,0,1,1,1,0,1,0,0,1,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,1,0,0,1,0,0,0,1,3,},
		},
		.recovery_ok = 1,
		.recovers_to = {},
	},
	/* TODO CSTAT Test cases for leap seconds */
};

static void run_xeliminate_testcases()
{
	unsigned char curtest;
	unsigned char i;
	unsigned char j;
	unsigned char bitval;
	unsigned char rv;
	unsigned char telegram[9][16];
	
	for(curtest = 0; curtest <
			(sizeof(xeliminate_testcases) /
			sizeof(struct xeliminate_testcase)); curtest++) {

		memset(telegram, 0, sizeof(telegram));

		for(i = 0; i < xeliminate_testcases[curtest].num_lines; i++) {
			for(j = 0; j <
			xeliminate_testcases[curtest].line_len[i]; j++) {
				switch(xeliminate_testcases[curtest].
								data[i][j]) {
				case 0:  bitval = 2; break;
				case 1:  bitval = 3; break;
				case 2:  bitval = 0; break;
				case 3:  bitval = 1; break;
				default: puts("<<<ERROR1>>>"); exit(64);
				}
				telegram[i][j / 4] |= bitval << ((j % 4) * 2);
			}
			/*
			printf("Telegram %d:    ", i);
			for(j = 0; j < sizeof(telegram_1)/sizeof(unsigned char); j++)
				printf("%02x,", telegram[i][j]);
			puts("");
			*/
		}
		rv = 1;
		for(i = 1; rv == 1 && i < xeliminate_testcases[curtest].num_lines; i++)
			rv = xeliminate(
				xeliminate_testcases[curtest].line_len[i - 1],
				xeliminate_testcases[curtest].line_len[i],
				telegram[i - 1],
				telegram[i]
			);

		if(rv == xeliminate_testcases[curtest].recovery_ok) {
			if(rv == 1 && memcmp(telegram[xeliminate_testcases[curtest].num_lines - 1],
					xeliminate_testcases[curtest].recovers_to, 15) != 0) {
				/* fail */
				printf("[FAIL] Test %d: %s -- telegram mismatch\n",
					curtest, xeliminate_testcases[curtest].description);
				printf("       Expected  ");
				for(j = 0; j < 15; j++)
					printf(
						"%02x,",
						xeliminate_testcases[curtest].
						recovers_to[j]
					);
				printf("\n       Got       ");
				for(j = 0; j < 15; j++)
					printf(
						"%02x,",
						telegram[xeliminate_testcases[
						curtest].num_lines - 1][j]
					);
				puts("");
			} else {
				/* pass */
				printf("[ OK ] Test %d: %s\n", curtest,
					xeliminate_testcases[curtest].description);
			}
		} else {
			/* fail */
			printf("[FAIL] Test %d: %s -- unexpected rv idx=%d\n", curtest,
					xeliminate_testcases[curtest].description, i);
		}
		
	}
}

/* ---------------------------------------------------[ Logic Declaration ]-- */

/* interface */
#define DCF77_HIGH_LEVEL_MEM   144 /* ceil(61*2/8) * 9 */
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

/* ------------------------------------------------[ Logic Implementation ]-- */
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
		printf("<<<ERROR2,%02x,%02x>>>\n", *in_telegram_1,
							*in_out_telegram_2);
		return 0;
	}

	etmp = read_entry(*in_out_telegram_2, 0);
	if(etmp == VAL_1) {
		puts("<<<ERROR3>>>");
		return 0; /* constant 0 violated */
	} else if(etmp == VAL_X) {
		/* correct to 0 */
		*in_out_telegram_2 = (*in_out_telegram_2 & ~3) | VAL_0;
	}

	/* 16--20: entries have to match */
	for(i = 16; i <= 20; i++) {
		if(!xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4)) {
			printf("<<<ERROR4,%d>>>\n", i);
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
	} else if(etmp == VAL_X) {
		/* unset => correct to 1 */
		in_out_telegram_2[5] = (in_out_telegram_2[5] & ~3) | VAL_1;
	}

	/* 25--58: entries have to match */
	for(i = 25; i <= 58; i++) {
		/*
		 * except for minute parity bit (28) which may change depending
		 * on minute unit value
		 */
		if(i == 28)
			continue;

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
		/* TODO z Though they are rare, leap seconds are an interesting opportunity to recover many things: We know that they only appear at the end of hours and thus can derive all minute bits (need to be BCD 0). Additionally, when seing a leap second, we should check bit 19 which needs to be 1 (or X respectively). In case the leap second is in the 2nd telegram this allows us to recover bit 19 (must be 1...)? One might want to implement such recovery mechanisms but only if they are tested very extensively because otherwise the clock will likely crash on the first leap second encountered w/o possibility of debugging?
		TODO CSTAT CURRENTLY FAILS W/ ERROR 4 [BIT 19 which is leap second marker mismatches] -> seems one should implement it with more than just this little comparison in the end? */
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
			read_entry(telleap[14],    3) == VAL_0) && /* marker */
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
