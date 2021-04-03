#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_secondlayer_check_bcd_correct_telegram.h"

struct checkbcd_test_case {
	char*         description;
	unsigned char expect_is_valid;
	unsigned char telegram[DCF77_SECONDLAYER_LINE_BYTES];
};

static struct checkbcd_test_case TEST_CASES[] = {
	/*
	 * Output from another test. This telegram is entirely invalid because
	 *  - it has wrong date parity
	 *  - 00 for day of month and
	 *  - 1101 for month ones
	 *  - incorrect end of minute (should be NO_SIGNAL)
	 * TODO CURRENTLY THE ERRORS ARE WRONG SHOULD FAIL THAT TELEGRAMS EARLIER!
	 */
	{"T1 invalid date parity fails check", 0,
		{0x55,0x55,0x55,0x55,0x55,0x55,0xa9,0xaa,0xaa,0xaa,0xea,0xbd,0xab,0xba,0xea,}},
	{"T1 unrealistic day of month fails check", 0,
		{0xa9,0xaa,0xaa,0x6a,0x55,0x55,0xa9,0xaa,0xaa,0xaa,0xea,0xef,0xaa,0xba,0xfa,}},
	/* TODO CSTAT THIS TEST CASE FAILS WHY */
	{"T1 corrected passes", 1, {0xa9,0xaa,0xaa,0x6a,0x55,0x55,0xa9,0xaa,0xaa,0xab,0xea,0xef,0xaa,0xba,0x6a,}},
};

#define NUM_TEST_CASES (sizeof(TEST_CASES)/sizeof(struct checkbcd_test_case))

int main(int argc, char** argv)
{
	struct dcf77_secondlayer ctx;
	unsigned int i;
	unsigned int j;
	unsigned char test_result;
	unsigned char has_fail = 0;
	for(i = 0; i < NUM_TEST_CASES; i++) {
		memcpy(ctx.private_telegram_data, TEST_CASES[i].telegram,
					sizeof(struct checkbcd_test_case));
		test_result = dcf77_secondlayer_check_bcd_correct_telegram(&ctx,
									0, 0);
		if(test_result == TEST_CASES[i].expect_is_valid) {
			printf("[ OK ] %s\n", TEST_CASES[i].description);
		} else {
			has_fail = 1;
			printf("[FAIL] %s -- expected %d, got %d for {",
				TEST_CASES[i].description,
				TEST_CASES[i].expect_is_valid, test_result);
			for(j = 0; j < DCF77_SECONDLAYER_LINE_BYTES; j++)
				printf("0x%02x,", TEST_CASES[i].telegram[j]);
			printf("0x00}\n");
		}
	}
	return has_fail? EXIT_FAILURE: EXIT_SUCCESS;
}
