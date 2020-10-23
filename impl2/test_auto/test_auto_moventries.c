#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_proc_moventries.h"

#define MAXIN 256

enum callproc {
	PROC_NONE = 0,
	/* arg0 = number of places to move */
	/* arg1 = line with leapsec */
	PROC_MOVE_LEFTWARDS = 1,
};

struct test_case_moventries {
	char title[80];
	unsigned char in_length;
	enum dcf77_bitlayer_reading in[MAXIN];
	enum callproc callproc; /* procedure ID */
	unsigned char arg0;     /* primary parameter */
	unsigned char arg1;     /* secondary parameter */
	unsigned char out[DCF77_SECONDLAYER_MEM];
};

static const struct test_case_moventries TESTS[] = {
	{
		.title = "Three bit arrive rtl, no advanced processing.",
		.in_length = 3,
		.in = { DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1 },
		.out = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x3f },
	},
	{
		.title = "Fifteen bit arrive rtl, no advanced processing.",
		.in_length = 15,
		.in = {
			DCF77_BIT_0, DCF77_BIT_0, DCF77_BIT_0, DCF77_BIT_0, /* 4 */
			DCF77_BIT_0, DCF77_BIT_0, DCF77_BIT_0, DCF77_BIT_0, /* 8 */
			DCF77_BIT_0, DCF77_BIT_0, DCF77_BIT_0, DCF77_BIT_0, /* 12 */
			DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1
		},
		.out = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xaa, 0xaa, 0xaa, 0x3f },
	},
	{
		.title = "Signal EOM for first line.",
		.in_length = 8,
		.in = {
			/* end of first line (01111111) -> 01 for no signal, 3x 11 for 1  */
			DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_NO_SIGNAL,
			/* next line */
			DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1
		},
		.out = {
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0, /* first line */
			0xff /* second line */
		},
	},
	{
		.title = "Three bit arrive ltr, no advanced processing.",
		.in_length = 4,
		.in = {
			/* suppose we immediately signal EOM. First line "quasi empty", next line will hold telegram */
			DCF77_BIT_NO_SIGNAL,
			DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1
		},
		.out = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x40, 0,
			0x3f
		},
	},
	{
		.title = "One real telegram appears 03.01.2016 21:09.",
		.in_length = 60,
		.in = {
			/* 03.01.2016 21:09:00 | ee,ee,ba,bf,ba,af,ab,ae,ba,af,fa,af,ea,bb,7a,55 */
			2, 3, 2, 3, 2, 3, 2, 3, 2, 2, 3, 2, 3, 3, 3, 2, 2, 2, 3, 2,
			3, 3, 2, 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
		},
		.out = {0xee,0xee,0xba,0xbf,0xba,0xaf,0xab,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a},
	},
	{
		.title = "First telegram part. bit mistaken for end: before mov 03.01.2016 21:05+21:06.",
		.in_length = 78,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/*                                                    v defunct, cause NL */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1,
			3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply exactly enough data to not cause automatic reorg */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 3, 2, */
		},
		.out = {
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x7a,0x00,
			0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0x0a,
		},
	},
	{
		.title = "First telegram part. bit mistaken for end after mov 03.01.2016 21:05+21:06.",
		.in_length = 78,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/*                                                    v defunct, cause NL */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1,
			3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply exactly enough data to not cause automatic reorg */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 3, 2, */
		},
		.out = {
			/* Matches the telegram for 03.01.2016 21:05:00 except for broken part 0x7a in place of 0xba */
			0xaa,0xaa,0xaa,0xaa,0x7a,0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,0x00,
			/* This data is not needed/will be overriden but that's how the memory looks like after the move */
			0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0x0a,
		},
		.callproc = PROC_MOVE_LEFTWARDS,
		.arg0 = 40, /* move 40 places s.t. ends on 0x7a */
		.arg1 = 9, /* noleap */
		/* NB: When this test was created, cursor ended at x=5, y=1; seems strange that it would not go to x=0? */
	},
	{
		.title = "First telegram part. bit mistaken for end auto mov 03.01.2016 21:05+21:06.",
		.in_length = 80,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/*                                                    v defunct, cause NL */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1,
			3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply enough data to cause one recompute_eom() */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			3, 2, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
		},
		.out = {
			/* Matches the telegram for 03.01.2016 21:05:00 except for broken part 0x7a in place of 0xba */
			0xaa,0xaa,0xaa,0xaa,0x7a,0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,0x00,
			/* This data is not needed/will be overriden but that's how the memory looks like after the move */
			0xef,0xaa,0xae,0xba,0xbf,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0xba
		},
	},
	{
		.title = "Like before 03.01.2016 21:05+21:06, but mov does not fit byte",
		.in_length = 82,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			/* v defunct, cause NL */
			3, 1, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply enough data to cause one recompute_eom() */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			3, 2, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
		},
		.out = {
			/* Matches the telegram for 03.01.2016 21:05:00 except for broken part */
			/*
				TODO CSTAT SUBSTAT: Below is the correct ocmparison data. However, we get a 7b instead 0f 7a when running the code. That must be a bug:
	
0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
Expected telegram "7a"
0000 0000 0000 0000 0010 1301 0000 0100 0010 1100 0011 1100 0001 1010 0013 3 "03.10.2016 21:0X:00"
Got telegram "7b"
0000 0000 0000 0000 0010 1301 0000 0100 0010 1100 0011 1100 0001 1010 1013 3 "03.01.2056 21:0X:00"
							              ^_ additional 1 should not be there
			*/
			0xaa,0xaa,0xaa,0xaa,0xba,0xe7,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,0x00,
			/* This data is not needed/will be overriden but that's how the memory looks like after the move */
			/*0xef,0xaa,0xae,0xba,0xbf,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0xba TBD*/
		},
	},
	/* TODO ASTAT/CSTAT NEXT DO SIMILAR TESTS TO DEBUG MOVE CASES, STATUS OF INVESTIGATION:

		-> move cases which are not divisable by 8, preferrably 39 because that seemed to wreck havoc in the data [TO BE VERIFIED BY TEST CASE] --------> WORK IN PROGRESS SEE ABOVE!

		-> leap second cases
		-> can we already process more than two telegrams with the current status of the program?
	Future:
	Afterwards work towards deprecating current contents of secondlayer test, because it will no longer be needed.
	One may still want a secondlayer test to check the outputs from functions rather than the memory contents which is what we are doing here, actually! Might also extend the format to not only have memory output comparing but also output data comparison?
	*/
};
#define NUMCASES (sizeof(TESTS)/sizeof(struct test_case_moventries))

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
}

int main(int argc, char** argv)
{
	struct dcf77_secondlayer test_ctx;
	unsigned char test_id;
	unsigned char i;

	for(test_id = 0; test_id < NUMCASES; test_id++) {
		/* run test */
		memset(&test_ctx, 0, sizeof(struct dcf77_secondlayer));
		dcf77_secondlayer_init(&test_ctx);
		for(i = 0; i < TESTS[test_id].in_length; i++) {
			test_ctx.in_val = TESTS[test_id].in[i];
			dcf77_secondlayer_process(&test_ctx);
		}
		/* invoke procedure */
		switch(TESTS[test_id].callproc) {
		case PROC_NONE:
			break;
		case PROC_MOVE_LEFTWARDS:
			dcf77_proc_move_entries_backwards(&test_ctx,
				TESTS[test_id].arg0, TESTS[test_id].arg1);
			break;
		default:
			printf("[ERRO] UNKNOWN PROCEDURE: %d\n",
						TESTS[test_id].callproc);
		}
		/* compare result */
		if(memcmp(test_ctx.private_telegram_data, TESTS[test_id].out,
						DCF77_SECONDLAYER_MEM) == 0) {
			printf("[ OK ] test_id=%d, %s\n", test_id,
							TESTS[test_id].title);
		} else {
			printf("[FAIL] test_id=%d, %s\n", test_id,
							TESTS[test_id].title);
			puts("Got");
			dumpmem(&test_ctx);
			puts("Expected");
			memcpy(&test_ctx.private_telegram_data,
				TESTS[test_id].out, DCF77_SECONDLAYER_MEM);
			dumpmem(&test_ctx);
		}
	}
	return EXIT_SUCCESS;
} 
