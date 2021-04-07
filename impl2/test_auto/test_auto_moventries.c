#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_secondlayer_moventries.h"

#define MAXIN 256

enum callproc {
	PROC_NONE = 0,

	/*
	 * called after having sent all data
	 * arg0 = number of places to move 
	 * arg1 = line with leapsec
	 */
	PROC_MOVE_LEFTWARDS = 1,

	/*
	 * called before sending the first bit.
	 * arg0 = value for leap sec announce in minutes (will be taken * 60)
	 */
	PROC_SET_LEAPSEC_ANNOUNCE = 2,
};

struct test_case_moventries {
	char title[80];
	unsigned char in_length;
	enum dcf77_bitlayer_reading in[MAXIN];

	enum callproc callproc; /* procedure ID */
	unsigned char arg0;     /* primary parameter */
	unsigned char arg1;     /* secondary parameter */

	unsigned char out[DCF77_SECONDLAYER_MEM]; /* expected memory content */
	/* expected telegram lengths (default 0/0) */
	unsigned char out_telegram_1_len;
	unsigned char out_telegram_2_len;
	/* expected telegram contents (if len != 0) */
	unsigned char out_telegram_1[DCF77_SECONDLAYER_LINE_BYTES];
	unsigned char out_telegram_2[DCF77_SECONDLAYER_LINE_BYTES];
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
			/* end of first line */
			DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_NO_SIGNAL,
			/* next line */
			DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1
		},
		.out = {
			0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x7b, /* first line */
			0xff /* second line */
		},
		/* Newly produces an output telegram although a very sparse one */
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0x55,0x57,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x7b, },
		.out_telegram_1_len = 60,
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
			0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
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
		.out_telegram_1_len = 60,
		.out_telegram_1 = {0x56,0x55,0x55,0x55,0xba,0xaf,0xab,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,},
	},
	{
		.title = "First telegram part. bit mistaken for end: before mov 03.01.2016 21:05+21:06.",
		.in_length = 78,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* |--------------| (replace 2->3 to avoid reorg)     v defunct, cause NL */
			2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1,
			3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply exactly enough data to not cause automatic reorg */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 3, 2, */
		},
		.out = {
			0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0xfe,0xbf,0xaa,0xaa,0x7a,
			0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0x0a,
		},
		.out_telegram_1_len = 60,
		.out_telegram_1 = {0x56,0x55,0x55,0x55,0x55,0x57,0x55,0x55,0x55,0x55,0xfe,0xbf,0xaa,0xaa,0x7a,},
		.out_telegram_2 = {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,},
	},
	{
		.title = "First telegram part. bit mistaken for end: after mov 03.01.2016 21:05+21:06.",
		.in_length = 78,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* |-- replaced --|                                   v defunct, cause NL */
			2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1,
			3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply exactly enough data to not cause automatic reorg */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 3, 2, */
		},
		.out = {
			/* Matches the telegram for 03.01.2016 21:05:00 except for broken part 0x7a in place of 0xba and fe/bf from 3 replacement */
			0xfe,0xbf,0xaa,0xaa,0x7a,0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
			/*        | This data is not needed/will be overriden but that's how the memory looks like after the move */
			0xaa,0xaa,0xaa,0xaa,0x0a,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0x0a,
		},
		.callproc = PROC_MOVE_LEFTWARDS,
		.arg0 = 40, /* move 40 places s.t. ends on 0x7a */
		/* .arg1 = 9, noleap | no longer evaluated? */
		/* NB: When this test was created, cursor ended at x=5, y=1; seems strange that it would not go to x=0? */
		/* Ignore output */
		.out_telegram_1_len = 60,
		.out_telegram_1 = {0x56,0x55,0x55,0x55,0x55,0x57,0x55,0x55,0x55,0x55,0xfe,0xbf,0xaa,0xaa,0x7a,},
		.out_telegram_2 = {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,},
	},
	{
		/* Newly moves by 1 earlier than prevously. Detects incorrectness too early :) */
		.title = "First telegram part. bit mistaken for end: auto mov 03.01.2016 21:05+21:06.",
		.in_length = 80,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* |-- replaced --|                                   v defunct, cause NL */
			2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 1,
			3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/* partial telegram, supply enough data to cause one recompute_eom() */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			3, 2, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
		},
		.out = {
			0xfe,0xbf,0xaa,0xaa,0x7a,0xef,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
			/*        |-> ignore/remainder from before mov */
			0xaa,0xaa,0xaa,0xaa,0xba,0xfa,0xaf,0xea,0xbb,0x7a,0xaa,0xaa,0xaa,0xaa,0xba
		},
		/* distorted 21:05 telegram output as 21:06 not received completely yet */
		.out_telegram_1 = {0x56,0x55,0x55,0x55,0x55,0x57,0x55,0x55,0x55,0x55,0xfe,0xbf,0xaa,0xaa,0x7a,},
		.out_telegram_1_len = 60,
		.out_telegram_2 = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55, },
	},
	{
		.title = "Like before 03.01.2016 21:05+21:06, but mov does not fit byte",
		.in_length = 82,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/*       |-- replaced --| */
			2, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
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
			0xea,0xff,0xab,0xaa,0xba,0xe7,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
			/* This data is not needed/will be overriden but that's how the memory looks like after the move */
			0xaa,0xaa,0xaa,0xaa,0xba,0x0b,0xaa,0xbe,0xab,0xa7,0xaa,0xaa,0xaa,0xaa,0xbb,
		},
		.out_telegram_1 = {0x56,0x55,0x55,0x55,0x55,0x57,0x55,0x55,0x55,0xa5,0xfe,0xbf,0xaa,0xaa,0x7b,},
		.out_telegram_1_len = 60,
		.out_telegram_2 = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55, },
	},
	{
		.title = "03.01.2016 21:05+21:06, mov does not fit byte, two full telegrams",
		.in_length = 120,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/*       |-- replaced --| */
			2, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			/* v defunct, cause NL */
			3, 1, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			3, 2, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
		},
		.out = {
			0xea,0xff,0xab,0xaa,0xba,0xe7,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
			0xaa,0xaa,0xaa,0xaa,0xba,0xfb,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
		},
		/* Correct output telegram 21:06 */
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0xba,0xfb,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a, },
		.out_telegram_1_len = 60,
		/* nb. out_telegram_2_len = 0 because this does not give new information: */
		.out_telegram_2 = { 0x56,0x55,0x55,0x55,0x7a,0x57,0xa9,0xad,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a, },
	},
	{
		.title = "03.01.2016 21:05+21:06+21:07 let mov recover actual correct output telegram",
		.in_length = 180,
		.in = {
			/* 03.01.2016 21:05:00 | aa,aa,aa,aa,ba,ef,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			/*       |-- replaced --| */
			2, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			/* v defunct, cause NL */
			3, 1, 2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:06:00 | aa,aa,aa,aa,ba,fb,aa,ae,ba,af,fa,af,ea,bb,7a,55 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			3, 2, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1,
			/* 03.01.2016 21:07:00 | */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,
			3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1
		},
		.out = {
			/* TBD */
			0xea,0xff,0xab,0xaa,0xba,0xe7,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
			0xaa,0xaa,0xaa,0xaa,0xba,0xfb,0xaa,0xae,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
			0xaa,0xaa,0xaa,0xaa,0xba,0xff,0xaa,0xaf,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a,
		},
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0xba,0xff,0xaa,0xaf,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a, },
		.out_telegram_1_len = 60,
		/* again nothing to learn from this, thus len = 0 */
		.out_telegram_2 = { 0x56,0x55,0x55,0x55,0x7a,0x57,0xa9,0xad,0xba,0xaf,0xfa,0xaf,0xea,0xbb,0x7a, },
	},
	{
		.title     = "Leapsec single tel: pre, in. Announce predef. Noproc",
		.callproc  = PROC_SET_LEAPSEC_ANNOUNCE,
		.arg0      = 3, /* should happen within next three minutes */
		.in_length = 100,
		.in        = {
			/* 01.07.2012 01:59:00, ankuend | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 (last 40b) */
			3, 3, 2, 2, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1,
			/* 01.07.2012 02:00:00, hasl    | aa,ef,ff,bb,ee,ab,aa,ba,ea,ab,fa,ff,ea,ba,ba,55  */
			2, 2, 2, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, /* 1, <- leap here */
		},
		.out = {
			/* Partial first telegram */
			0x55,0x55,0x55,0x55,0x55,0xaf,0xef,0xae,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
			/* Second telegram except for leap part */
			0xaa,0xef,0xff,0xbb,0xee,0xab,0xaa,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0xba
		}
	},
	{
		.title     = "Leapsec single tel: pre, in. Announce predef. Shallproc.",
		.callproc  = PROC_SET_LEAPSEC_ANNOUNCE,
		.arg0      = 3, /* should happen within next three minutes */
		.in_length = 101,
		.in        = {
			/* 01.07.2012 01:59:00, ankuend | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 (last 40 bits) */
			3, 3, 2, 2, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1,
			/* 01.07.2012 02:00:00, hasl    | aa,ef,ff,bb,ee,ab,aa,ba,ea,ab,fa,ff,ea,ba,ba,55 */
			2, 2, 2, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, 1, /* <- leap here */
		},
		.out = {
			/* Partial first telegram */
			0x55,0x55,0x55,0x55,0x55,0xaf,0xef,0xae,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
			/* Second telegram including leap part */
			0xaa,0xef,0xff,0xbb,0xee,0xab,0xaa,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0xba,
		},
		.out_telegram_1_len = 61,
		.out_telegram_2_len = 60,
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0xee,0xab,0xaa,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
		.out_telegram_2 = { 0x56,0x55,0x55,0x55,0x55,0x57,0xed,0xad,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
	},
	{
		.title = "Pre leapsec telegram. Start at 0. Full telegram. Shallproc.",
		.in_length = 60,
		.in = {
			/* 01.07.2012 01:59:00, ankuend | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 */
			2, 3, 3, 3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 3, 2, 2, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1,
		},
		.out = {
			/* Memory = input telegram */
			0xfe,0xae,0xee,0xbb,0xee,0xaf,0xef,0xae,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
		},
		.out_telegram_1_len = 60,
		.out_telegram_1 = {
			/* out telegram = in telegram except for weather bits */
			0x56,0x55,0x55,0x55,0xee,0xaf,0xef,0xae,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
		 }
	},
	{
		.title = "Leapsec 2 full telegram: pre, in.",
		.in_length = 121,
		.in = {
			/* 01.07.2012 01:59:00, ankuend | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 */
			2, 3, 3, 3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 3, 2, 2, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1,
			/* 01.07.2012 02:00:00, hasl    | aa,ef,ff,bb,ee,ab,aa,ba,ea,ab,fa,ff,ea,ba,ba,55  */
			2, 2, 2, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, 1, /* <- leap here */
		},
		.out = {
			0xfe,0xae,0xee,0xbb,0xee,0xaf,0xef,0xae,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
			0xaa,0xef,0xff,0xbb,0xee,0xab,0xaa,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0xba,
		},
		.out_telegram_1_len = 61,
		.out_telegram_2_len = 60,
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0xee,0xab,0xaa,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
		.out_telegram_2 = { 0x56,0x55,0x55,0x55,0x6e,0x57,0xed,0xad,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
	},
	{
		.title = "Leapsec 3 full telegram: pre, in, post.",
		.in_length = 181,
		.in = {
			/* 01.07.2012 01:59:00, ankuend  | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 */
			2, 3, 3, 3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 3, 2, 2, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1,
			/* 01.07.2012 02:00:00, hasl     | aa,ef,ff,bb,ee,ab,aa,ba,ea,ab,fa,ff,ea,ba,ba,55 */
			2, 2, 2, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, 1, /* <- leap here */
			/* 01.07.2012 02:01:00, postleap | ba,ee,fe,bb,ae,af,aa,bb,ea,ab,fa,ff,ea,ba,7a,55 */
			2, 2, 3, 2, 2, 3, 2, 3, 2, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 2,
			3, 3, 2, 2, 2, 2, 2, 2, 3, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1,
		},
		.out = {
			0xfe,0xae,0xee,0xbb,0xee,0xaf,0xef,0xae,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
			0xaa,0xef,0xff,0xbb,0xee,0xab,0xaa,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0xba,
			0xba,0xee,0xfe,0xbb,0xae,0xaf,0xaa,0xbb,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a,
		},
		.out_telegram_1_len = 60,
		.out_telegram_2_len = 60,
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0xae,0xaf,0xaa,0xbb,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
		.out_telegram_2 = { 0x56,0x55,0x55,0x55,0x6e,0x57,0xed,0xad,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
	},
	{
		.title = "Leapsec 2 miasligned distorted: pre, in. PPreparatory.",
		.in_length = 119,
		.in = {
			/* 01.07.2012 01:59:00, ankuend  | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 */
			/*                 eq 011101000101101001011100130101000001100000133111000300300333 */
			2, 3, 3, 3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			/*             v */
			3, 3, 2, 2, 3, 1, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 1, 1, 3, 3, 3, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 1,
			/* 01.07.2012 02:00:00, hasl     | aa,ef,ff,bb,ee,ab,aa,ba,ea,ab,fa,ff,ea,ba,ba,55 */
			2, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 3, 2, 2, 1, 2, 1,
			1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, 1, /* <- leap here */
		},
		.out = {
			0xfe,0xae,0xee,0xbb,0xee,0xaf,0xe7,0xae,0xea,0xab,0x7a,0xfd,0x6a,0x9a,0x56,
			0xa6,0x65,0x55,0xb9,0x66,0xa5,0xa5,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0x3a,
		},
		.out_telegram_1_len = 60,
		/* distorted telegram from first processing */
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0xee,0xaf,0xe7,0xae,0xea,0xab,0x7a,0xfd,0x6a,0x9a,0x56, },
		.out_telegram_2 = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55, },
	},
	{
		.title = "Leapsec 2 miasligned distorted: pre, in. Withleap.",
		.in_length = 121,
		.in = {
			/* 01.07.2012 01:59:00, ankuend  | fe,ae,ee,bb,ee,af,ef,ae,ea,ab,fa,ff,ea,ba,7a,55 */
			/*                 eq 011101000101101001011100130101000001100000133111000300300333 */
			2, 3, 3, 3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 3,
			/*             v */
			3, 3, 2, 2, 3, 1, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 1, 1, 3, 3, 3, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 1,
			/* 01.07.2012 02:00:00, hasl     | aa,ef,ff,bb,ee,ab,aa,ba,ea,ab,fa,ff,ea,ba,ba,55 */
			2, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 3, 2, 2, 1, 2, 1,
			1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2,
			2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, 1, /* <- leap here */
		},
		.out = {
			0xfe,0xae,0xee,0xbb,0xee,0xaf,0xe7,0xae,0xea,0xab,0x7a,0xfd,0x6a,0x9a,0x56,
			0xa6,0x65,0x55,0xb9,0x66,0xa5,0xa5,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0xba,
		},
		.out_telegram_1_len = 61,
		/* Recovers all-correctly (very nice, would not have expected that to work?) */
		.out_telegram_1 = { 0x56,0x55,0x55,0x55,0x6e,0xa7,0xa5,0xba,0xea,0xab,0xfa,0xff,0xea,0xba,0x7a, },
		.out_telegram_2_len = 60,
		.out_telegram_2 = { 0x56,0x55,0x55,0x55,0x6e,0x57,0xe5,0xad,0xea,0xab,0x7a,0xfd,0x6a,0x9a,0x56, },
	},
};
#define NUMCASES (sizeof(TESTS)/sizeof(struct test_case_moventries))

static void printtel_sub(unsigned char* data)
{
	unsigned char j;
	for(j = 0; j < 15; j++)
		if(data[j] == 0x00)
			printf("__,");
		else
			printf("%02x,", data[j]);
	/*printf("(%02x)", data[15]);*/
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
	printf("    [DEBUG] out_telegram_1 len=%2u: ", ctx->out_telegram_1_len);
	printtel_sub(ctx->out_telegram_1);
	printf("    [DEBUG] out_telegram_2 len=%2u: ", ctx->out_telegram_2_len);
	printtel_sub(ctx->out_telegram_2);
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
		/* invoke pre-procedure */
		switch(TESTS[test_id].callproc) {
		case PROC_SET_LEAPSEC_ANNOUNCE:
			test_ctx.private_leap_second_expected =
						TESTS[test_id].arg0 * 60;
			break;
		default: /* pass */
			break;
		}
		/* send data */
		for(i = 0; i < TESTS[test_id].in_length; i++) {
			test_ctx.in_val = TESTS[test_id].in[i];
			dcf77_secondlayer_process(&test_ctx);
			if(test_id == 160)
				dumpmem(&test_ctx); /* -- large debug output */
		}
		/* invoke post-procedure */
		switch(TESTS[test_id].callproc) {
		case PROC_MOVE_LEFTWARDS:
			dcf77_secondlayer_move_entries_backwards(&test_ctx,
							TESTS[test_id].arg0);
			break;
		default: /* pass */
			break;
		}
		/* compare result */
		if(
			memcmp(test_ctx.private_telegram_data,
				TESTS[test_id].out, DCF77_SECONDLAYER_MEM) == 0
			&&
			(TESTS[test_id].out_telegram_1_len == 0 || (
				test_ctx.out_telegram_1_len ==
					TESTS[test_id].out_telegram_1_len
				&& 
				memcmp(test_ctx.out_telegram_1,
					TESTS[test_id].out_telegram_1,
					DCF77_SECONDLAYER_LINE_BYTES) == 0))
			&&
			(TESTS[test_id].out_telegram_2_len == 0 || (
				test_ctx.out_telegram_2_len ==
					TESTS[test_id].out_telegram_2_len
				&& 
				memcmp(test_ctx.out_telegram_2,
					TESTS[test_id].out_telegram_2,
					DCF77_SECONDLAYER_LINE_BYTES) == 0))
		) {
			printf("[ OK ] test_id=%d, %s\n", test_id,
							TESTS[test_id].title);
		} else {
			printf("[FAIL] test_id=%d, %s\n", test_id,
							TESTS[test_id].title);
			puts("Got");
			dumpmem(&test_ctx);
			puts("Expected");
			test_ctx.out_telegram_1_len =
					TESTS[test_id].out_telegram_1_len;
			test_ctx.out_telegram_2_len =
					TESTS[test_id].out_telegram_2_len;
			memcpy(&test_ctx.private_telegram_data,
				TESTS[test_id].out, DCF77_SECONDLAYER_MEM);
			memcpy(&test_ctx.out_telegram_1,
				TESTS[test_id].out_telegram_1,
				DCF77_SECONDLAYER_LINE_BYTES);
			memcpy(&test_ctx.out_telegram_2,
				TESTS[test_id].out_telegram_2,
				DCF77_SECONDLAYER_LINE_BYTES);
			dumpmem(&test_ctx);
		}
	}
	return EXIT_SUCCESS;
} 
