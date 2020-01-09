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
	/* TODO CSTAT TEST FAILS PRODUCES A 3 (NO SIGNAL) instead of a 1 (BIT_1) / FIND OUTHOW IT FINDS THE BIT TO WRITE TO TARGET... | REASON: The first line to be processed is set to idx=0 ... maybe it does not read from the "next" line (idx=1) / ES IST IRGENDWAS MIT EINEM MISMATCH ZWISCHEN DER INPUT LINE UND DER OUTPUT LINE BZW. ES WIRD AUS DER FALSCHEN "RICHTUNG" GELESEN??? Normalerweise fängt man an mit der "aktuellen" Zeile und verschiebt rückwärts? Die Implementierung ist aktuell aber anders? CHCK W/ PLAN / MAYBE IT ACTUALLY WORKS THIS WAY BUT THE TEST IS WRONG BECAUSE THE "CURRENT LINE" IS NOT TAKEN INTO CONSIDERATION "on purpose"? / We should be able to resolve this by supplying two full lines and then move backwards ~> changes should occur in the full lines. The current line might as well remain untouched? / Check on how the procedure is used if this signifies some changes are necessary? / YES: The key is recompute_eom()'s preconditions clearly specify that we have just finished a current line before calling the movement procedure. Thus pls adjust test case to supply one full line which does not end on proper EOM and then invoke movement! For this, we can actually do exactly as tested here there is no expectation that the bits from the "next line" are touched in any way!!! Current behaviour for this test is correct!!! / PROBLEM: NEED TO SUPPLY AT LEAST TWO LINES BECAUSE THE CURRENT VARIANT REALLY DOES NOT MAKE SENSE... */
	{
		.title = "Send two lines of pattern 1010, no special processing",
		.in_length = 62,
		.in = {
			/* first line almost empty */
			DCF77_BIT_1, DCF77_BIT_NO_SIGNAL,

			/* second line with pattern */
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 

			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 

			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 

			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_NO_SIGNAL, /* needs to end on no signal */
		},
		/* TODO Z TEST WORKS ONLY AS LONG AS PROCESS TELEGRAMS REMAINS DISABLED... */
		.out = {
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0,
			0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0x6e,
		},
	},
	{
		.title = "Send two lines of pattern 1010, move backwards by one",
		.in_length = 62,
		.in = {
			/* first line almost empty */
			DCF77_BIT_1, DCF77_BIT_NO_SIGNAL,

			/* second line with pattern */
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 

			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 

			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 

			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_1, 
			DCF77_BIT_0, DCF77_BIT_1, DCF77_BIT_0, DCF77_BIT_NO_SIGNAL, /* needs to end on no signal */
		},
		.callproc = PROC_MOVE_LEFTWARDS,
		.arg0 = 1, /* move by one place (2 bits) */
		.arg1 = 9, /* noleap */
		/* TODO TEST WORKS ONLY AS LONG AS PROCESS TELEGRAMS REMAINS DISABLED... CSTAT TEST FAILS MISERABLY MIGHT AGAIN BE DUE TO THE TELEGRAMS BEING INVALID :( BECAUSE MOVE LEFTWARDS NEED NOT DO ANYTHING SPECIAL AS IT "KNOWS" THAT A NO SIGNAL NEEDS TO BE MOVED UPWARDS ETC... JUST SO DAMN DIFFICULT TO TEST :(  / IT SEEMS ONE CANNOT DO SYNTHETICALLY. JUST DO WITH ACTUAL AND REAL TEST DATA AND DEBUG IT :( :( / GIVE UP ON INDIVIDUAL PROCEDURE TESTING? OR REALLY FIND SOME CASE THAT SATISFIES ALL THE CONDITIONS. MABE IT IS BETTER TO SUPPLY THE INPUT MEMORY MANUALLY RATHER THAN RELYING ON THE PROCESS FUNCTION TO FILL IT / BECAUSE IT HAS SO MANY DETAILS TO BE AWARE OF. TEST THE TWO THINGS ENTIRELY SEPARATELY. DO NOT PREPARE DATA BY USING THE REGULAR FUNCTION BECAUSE IT WILL ONLY WORK WITH ACTUAL VALID DATA! */
		/*
		TODO CSTAT HERE IS THE TEST REORGANIZAZION
		 * SECONDLAYER TEST: SPLIT IN "RECOVERY" TEST WHICH USES EXISTING TELEGRAMS (LIKE NOW) + ONE "SIMPLE" test which is similar to what I have in this moventries file except it does not ever invoke moventries. Once all these tests pass.
		 * Create two separate moventries tests: One with actual real data that flows to the process functions and one (first one) with memory input/output.

		 * Thus do as follows
			1. Clean up the mess. Remove existing moventries and secondlayer tests. Remove unwanted test exports from the files from the real implementation.
			2. Create secondlayer tests secondlayer_simple.c: All the cases without reorganizazion (secondlayer_simple.c)
			3. Create secondlayer tests with existing telegrams: secondlayer_recovery.c: All test cases without reorganizazion should pass. All others may fail freely.
			4. Create moventries tests with input memories s.t. all the secondary conditions can be ignored which are not necessary for the movement procedure. This test is moventries_simple.c.
			5. After fixing all the moventries_simple cases, the existing telegrams tests should also pass for the cases with moventries involved.
		*/
		.out = {
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0,
			0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee,0x6e,
		},
	},
		#if 0
		#endif
	#if 0
	{
		/* DOES NOT WORK BLK FIX OTHER FIRST */
		.title = "Three bit arrive ltr, then move backwards by two places.",
		.in_length = 4,
		.in = {
			DCF77_BIT_NO_SIGNAL,
			DCF77_BIT_1, DCF77_BIT_1, DCF77_BIT_1
		},
		.callproc = PROC_MOVE_LEFTWARDS,
		.arg0 = 2, /* move by two places (4 bits) */
		.arg1 = 9, /* noleap */
		.out = {
			/* should end on 2311 which means 01 (no signal/3), 11 (1), 11 (1) ~> last is 11_11_01_00 = 0xf4 */
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xf4, 0,
			/* next line should be reduced to single 11 = 00_00_00_11 = 0x03 */
			0x03
		},
	},
	#endif
};
#define NUMCASES (sizeof(TESTS)/sizeof(struct test_case_moventries))

static void printtel_sub(unsigned char* data)
{
	unsigned char j;
	for(j = 0; j < 15; j++)
		printf("%02x,", data[j]);
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
		/* memset(&test_ctx, 0, sizeof(struct dcf77_secondlayer)); */
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
