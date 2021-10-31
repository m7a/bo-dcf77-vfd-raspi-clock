#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_timelayer.h"

static char test_are_ones_compatible();
static char test_is_leap_year();
static char test_advance_tm_by_sec();
static char test_recover_ones();

int main(int argc, char** argv)
{
	/* use bitwise or here to ensure all tests are excecuted */
	return (test_are_ones_compatible() &
		test_is_leap_year() &
		test_advance_tm_by_sec() &
		test_recover_ones())?
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
			printf("[ OK ] test %2d compatible(%02x,%02x)=%d\n", i,
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
			printf("[ OK ] test %2d is leap %d\n", i,
							samples_leap_years[i]);
		} else {
			printf("[FAIL] test %2d is leap but detected as "
				"regular: %d\n", i, samples_leap_years[i]);
			test_pass = 0;
		}
	}
	for(i = 0; i < sizeof(samples_regular_years)/sizeof(short); i++) {
		if(!dcf77_timelayer_is_leap_year(samples_regular_years[i])) {
			printf("[ OK ] test %2d is regular: %d\n", i,
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

static char test_advance_tm_by_sec()
{
	const static struct dcf77_timelayer_tm input_tm[] = {
		{ 2021, 10, 23, 22, 28, 30 }, /* test 1: identity */
		{ 2021, 10, 23, 22, 28, 30 }, /* test 2: trivial */
		{ 2021, 10, 23, 19, 57, 13 }, /* test 3: inc hours */
		{ 2021,  9, 30, 23, 59,  0 }, /* test 4: daywrap 30 */
		{ 2021,  7, 31, 23, 59, 59 }, /* test 5: daywrap 31 */
		{ 2020, 12, 31, 23, 40, 15 }, /* test 6: yearwrap */
		{ 2021,  2, 28, 22,  0,  0 }, /* test 7: daywrap nonleap */
		{ 2020,  2, 28, 22,  0,  0 }, /* test 8: daywrap leap */
		{ 2020,  2, 29, 23, 17, 49 }, /* test 9: daywrap inleap */
		/* -- out of range -- */
		/* { 2021, 10, 23, 18, 57, 13 }, * test y: inc hours */
		/* { 2021, 10, 23, 14, 30, 21 }, * test y: inc hours */
	};
	const static short delta[] = {
		0,
		13,
		9189,
		60,
		11,
		1185, /* = 45+19*60 */
		7243,
		7243,
		2621, /* = (60-17-1)*60+(60-49)+90 */
		/* -- out of range -- */
		/* 12789, */
		/* 28801, */
	};
	const static struct dcf77_timelayer_tm output_tm[] = {
		{ 2021, 10, 23, 22, 28, 30 },
		{ 2021, 10, 23, 22, 28, 43 },
		{ 2021, 10, 23, 22, 30, 22 },
		{ 2021, 10,  1,  0,  0,  0 },
		{ 2021,  8,  1,  0,  0, 10 },
		{ 2021,  1,  1,  0,  0,  0 },
		{ 2021,  3,  1,  0,  0, 43 },
		{ 2020,  2, 29,  0,  0, 43 },
		{ 2020,  3,  1,  0,  1, 30 },
		/* -- out of range -- */
		/* { 2021, 10, 23, 22, 30, 22 }, */
		/* { 2021, 10, 23, 22, 30, 22 }, */
	};
	char test_pass = 1;
	unsigned char i;
	struct dcf77_timelayer_tm curo;
	puts("begin  test_advance_tm_by_sec...");
	for(i = 0; i < sizeof(delta)/sizeof(short); i++) {
		curo = input_tm[i];
		dcf77_timelayer_advance_tm_by_sec(&curo, delta[i]);
		if(memcmp(&curo, output_tm + i,
				sizeof(struct dcf77_timelayer_tm)) == 0) {
			printf("[ OK ] test %d: %04d-%02d-%02d %02d:%02d:%02d"
				" + %4d = %04d-%02d-%02d %02d:%02d:%02d\n",
				i + 1, input_tm[i].y, input_tm[i].m,
				input_tm[i].d, input_tm[i].h, input_tm[i].i,
				input_tm[i].s, delta[i], output_tm[i].y,
				output_tm[i].m, output_tm[i].d, output_tm[i].h,
				output_tm[i].i, output_tm[i].s);
		} else {
			test_pass = 0;
			printf("[FAIL] test %d: Expected %04d-%02d-%02d "
				"%02d:%02d:%02d + %4d to be %04d-%02d-%02d "
				"%02d:%02d:%02d, but got %04d-%02d-%02d "
				"%02d:%02d:%02d.\n", i + 1, input_tm[i].y,
				input_tm[i].m, input_tm[i].d, input_tm[i].h,
				input_tm[i].i, input_tm[i].s, delta[i],
				output_tm[i].y, output_tm[i].m, output_tm[i].d,
				output_tm[i].h, output_tm[i].i, output_tm[i].s,
				curo.y, curo.m, curo.d, curo.h, curo.i, curo.s);
		}
	}
	puts("end    test_advance_tm_by_sec.");
	return test_pass;
}

static char test_recover_ones()
{
	puts("begin  test_recover_ones...");
	/* see dcf77_timelayer.c declarations */
	static const unsigned char bcd[] = {
		0xaa, 0xab, 0xae, 0xaf, 0xba, 0xbb, 0xbe, 0xbf, 0xea, 0xeb,
	};
	/* test data */
	const static unsigned char preceding_minute_idx[] = {
		/* test 1-3: recover from all-complete BCDs */
		0,
		1,
		6,
		/* test 4: unable to recover if everything is "empty" */
		0,
		/* test 5: unable to recover if inconsistent data */
		0,
		/* test 6,7,8: recovers if at least two BCDs are complete */
		0,
		0,
		0,
		/* test 9: recovers with a single complete BCD */
		0,
		/* test 10: recovers 1 for offset 1 (single complete BCD) */
		0,
		/* test 11: recovers 0 for offset 1 + 1 (single complete BCD) */
		1,
		/* TODO ASTAT ONWARDS WITH TESTS OF INCOMPLETE BCDs! */
	};
	const static unsigned char preceding_minute_ones[][
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN] = {
		{ bcd[0], bcd[1], bcd[2], bcd[3], bcd[4],
		  bcd[5], bcd[6], bcd[7], bcd[8], bcd[9] },
		{ bcd[0], bcd[1], bcd[2], bcd[3], bcd[4],
		  bcd[5], bcd[6], bcd[7], bcd[8], bcd[9] },
		{ bcd[0], bcd[1], bcd[2], bcd[3], bcd[4],
		  bcd[5], bcd[6], bcd[7], bcd[8], bcd[9] },
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 },
		{ bcd[0], bcd[0], 0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 },
		{ bcd[0], bcd[1], 0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 },
		{ 0x55,   0x55,   bcd[2], bcd[3], 0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 },
		{ 0x55,   bcd[1], 0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   bcd[8], 0x55 },
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   bcd[8], 0x55 },
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   bcd[8], 0x55,   0x55 },
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   bcd[0] },
	};
	const static char expected_minute_recovery_result[] = {
		0,
		1,
		6,
		-1,
		-1,
		0,
		0,
		0,
		0,
		1,
		2,
	};
	/* end test data */
	char test_pass = 1;
	char recovery_result;
	unsigned char i;
	struct dcf77_timelayer ctx;
	for(i = 0; i < sizeof(preceding_minute_idx)/sizeof(unsigned char);
									i++) {
		ctx.private_preceding_minute_idx = preceding_minute_idx[i];
		memcpy(ctx.private_preceding_minute_ones,
			preceding_minute_ones[i],
			DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN);
		recovery_result = dcf77_timelayer_recover_ones(&ctx);
		if(recovery_result == expected_minute_recovery_result[i]) {
			printf("[ OK ] test %2d: Recoverd %2d as expected.\n",
				i + 1, recovery_result);
		} else {
			printf("[FAIL] test %2d: Expected %2d but got %2d.\n",
				i + 1, expected_minute_recovery_result[i],
				recovery_result);
			test_pass = 0;
		}
	}
	puts("end    test_recover_ones.");
	return test_pass;
}
