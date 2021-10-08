#include <stdio.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"

static char test_are_ones_compatible();
static char test_is_leap_year();

int main(int argc, char** argv)
{
	/* use bitwise or here to ensure all tests are excecuted */
	return (test_are_ones_compatible() & test_is_leap_year())?
		EXIT_SUCCESS: EXIT_FAILURE;
}

static char test_are_ones_compatible()
{
	const static unsigned char test_cases[][3] = {
		/* some arbitrary values are equivalent */
			/* 0 */
			{ 0,    0,    1 },
			/* 1 */
			{ 0xae, 0xae, 1 },
		/* cases where bits should be respected */
			/* 2 fails to recover */
			{ DCF77_BIT_0 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			DCF77_BIT_1 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			0 },
		/* cases where some bits are unset */
			/* 3 succeeds despite seeming to mismatch */
			{ DCF77_BIT_0 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			DCF77_BIT_NO_SIGNAL << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			1 },
			/* 4 */
			{ DCF77_BIT_0 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			DCF77_BIT_NO_UPDATE << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			1 },
			/* 5 */
			{ DCF77_BIT_NO_UPDATE << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			DCF77_BIT_1 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			1 },
		/* mixed cases */
			/* 6 */
			{ DCF77_BIT_NO_UPDATE << 6 | DCF77_BIT_NO_UPDATE << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			DCF77_BIT_1 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			1 },
			/* 7 */
			{ DCF77_BIT_NO_UPDATE << 6 | DCF77_BIT_NO_UPDATE << 4 |
				DCF77_BIT_1 << 2 | DCF77_BIT_0,
			DCF77_BIT_1 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			0 },
			/* 8 */
			{ DCF77_BIT_NO_UPDATE << 6 | DCF77_BIT_NO_UPDATE << 4 |
				DCF77_BIT_0 << 2 | DCF77_BIT_0,
			DCF77_BIT_1 << 6 | DCF77_BIT_0 << 4 |
				DCF77_BIT_1 << 2 | DCF77_BIT_0,
			0 },
	};
	const static unsigned char num_test_cases = sizeof(test_cases)/3;

	char test_pass = 1;
	unsigned char i;
	unsigned char rv;

	puts("begin  test_are_ones_compatible...");
	for(i = 0; i < num_test_cases; i++) {
		rv = dcf77_timelayer_are_ones_compatible(test_cases[i][0],
							test_cases[i][1]);
		if(rv == test_cases[i][2]) {
			printf("[PASS] test %2d compatible(%02x,%02x)=%d\n", i,
					test_cases[i][0], test_cases[i][1], rv);
		} else {
			printf("[FAIL] test %2d compatible(%02x,%02x)=%d but "
					"expected %d\n", i, test_cases[i][0],
					test_cases[i][1], rv, test_cases[i][2]);
			test_pass = 0;
		}
	}
	puts("end    test_are_ones_compatible.");
	return test_pass;
}

static char test_is_leap_year()
{
	char test_pass = 1;
	const static short samples_leap_years[] = {
		2000, 2020, 2024, 2028
	};
	const static short samples_regular_years[] = {
		1900, 2010, 2005
	};
	unsigned char i;
	puts("begin  test_is_leap_year...");
	for(i = 0; i < sizeof(samples_leap_years)/sizeof(short); i++) {
		if(dcf77_timelayer_is_leap_year(samples_leap_years[i])) {
			printf("[PASS] test %2d is leap %d\n", i,
							samples_leap_years[i]);
		} else {
			printf("[FAIL] test %2d is leap but detected as "
				"regular: %d\n", i, samples_leap_years[i]);
			test_pass = 0;
		}
	}
	for(i = 0; i < sizeof(samples_regular_years)/sizeof(short); i++) {
		if(!dcf77_timelayer_is_leap_year(samples_regular_years[i])) {
			printf("[PASS] test %2d is regular: %d\n", i,
						samples_regular_years[i]);
		} else {
			printf("[FAIL] test %2d is regular but detected as "
				"leap: %d\n", i, samples_regular_years[i]);
			test_pass = 0;
		}
	}
	puts("end    test_is_leap_year.");
	return test_pass;
}
