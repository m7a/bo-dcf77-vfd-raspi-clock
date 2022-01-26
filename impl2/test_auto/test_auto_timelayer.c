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
static char test_decode();
static char test_telegram_identity();
static char test_quality_of_service();

int main(int argc, char** argv)
{
	/* use bitwise or here to ensure all tests are excecuted */
	return (test_are_ones_compatible() &
		test_is_leap_year() &
		test_advance_tm_by_sec() &
		test_recover_ones() &
		test_decode() &
		test_telegram_identity() &
		test_quality_of_service())?
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
		/* -- procedure does not handle this -- */
		/* { 2021, 31, 10,  2, 59,  0 }, * test y: DST + */ 
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
		/* -- procedure does not handle this -- */
		/* -3600, */
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
		/* -- procedure does not handle this -- */
		/* { 2021, 31, 10,  1, 59,  0 }, */
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
		/* test 12: sequence 0000 7x_ ____ will not recover */
		0,
		/* test 13: sequence 0000 7x_ 1___ is for 0..8 */
		0,
		/* test 14: sequence 0000 8x_ 1___ is for 0..9 */
		0,
		/* test 15: sequence 0000 7x_ 1___ 1___ is for 0..9 */
		0,
		/* test 16: sequence 010_ ___1 is for 4..5 */
		0,
		/* test 17: sequence 0___ 1___ is for 7..8 */
		0,
	};
	const static unsigned char preceding_minute_ones[][
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN] = {
		{ bcd[0], bcd[1], bcd[2], bcd[3], bcd[4],
		  bcd[5], bcd[6], bcd[7], bcd[8], bcd[9] }, /* 1 */
		{ bcd[0], bcd[1], bcd[2], bcd[3], bcd[4],
		  bcd[5], bcd[6], bcd[7], bcd[8], bcd[9] }, /* 2 */
		{ bcd[0], bcd[1], bcd[2], bcd[3], bcd[4],
		  bcd[5], bcd[6], bcd[7], bcd[8], bcd[9] }, /* 3 */
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 }, /* 4 */
		{ bcd[0], bcd[0], 0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 }, /* 5 */
		{ bcd[0], bcd[1], 0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 }, /* 6 */
		{ 0x55,   0x55,   bcd[2], bcd[3], 0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55 }, /* 7 */
		{ 0x55,   bcd[1], 0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   bcd[8], 0x55 }, /* 8 */
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   bcd[8], 0x55 }, /* 9 */
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   bcd[8], 0x55,   0x55 }, /* 10 */
		{ 0x55,   0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   bcd[0] }, /* 11 */
		{ bcd[0], 0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55, }, /* 12 */
		{ bcd[0], 0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0xd5,   0x55, }, /* 13 */
		{ bcd[0], 0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0xd5, }, /* 14 */
		{ bcd[0], 0x55,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0xd5,   0xd5, }, /* 15 */
		{ 0xb9,   0x57,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55, }, /* 16 */
		{ 0x95,   0xd5,   0x55,   0x55,   0x55,
		  0x55,   0x55,   0x55,   0x55,   0x55, }, /* 17 */
	};
	const static char expected_minute_recovery_result[] = {
		0, /* 1 */
		1, /* 2 */
		6, /* 3 */
		-1, /* 4 */
		-1, /* 5 */
		0, /* 6 */
		0, /* 7 */
		0, /* 8 */
		0, /* 9 */
		1, /* 10 */
		2, /* 11 */
		0, /* 12 */
		0, /* 13 */
		0, /* 14 */
		0, /* 14 */
		4, /* 15 */
		7, /* 16 */
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

static char test_decode()
{
	const static struct dcf77_timelayer_tm recovered_tm[] = {
		/* test 1: synthetic */
		{.y=2021,.m=9,.d=11,.h=0,.i=24,.s=0,},
		/* test 2: real */
		{.y=2019,.m=4,.d=22,.h=22,.i=41,.s=0,},
	};
	const static unsigned char telegrams[][DCF77_SECONDLAYER_LINE_BYTES] = {
		{0xaa,0xaa,0xaa,0xaa,0xae,0xeb,0xba,0xaa,0xaa,0xab,0xeb,0xaf,0xbb,0xea,0x6a,},
		{0xaa,0xab,0xfb,0xaa,0xae,0xaf,0xea,0xba,0xba,0xae,0xbe,0xea,0xba,0xbe,0x7a,},
	};
	char test_pass = 1;
	unsigned char i;
	struct dcf77_timelayer_tm current_recover;
	puts("begin  test_decode...");
	for(i = 0; i < sizeof(recovered_tm)/sizeof(struct dcf77_timelayer_tm);
									i++) {
		dcf77_timelayer_decode(&current_recover, telegrams[i]);
		if(memcmp(&current_recover, recovered_tm + i,
				sizeof(struct dcf77_timelayer_tm)) == 0) {
			printf("[ OK ] test %2d -- %04d-%02d-%02d "
				"%02d:%02d:%02d\n", i + 1,
				recovered_tm[i].y, recovered_tm[i].m,
				recovered_tm[i].d, recovered_tm[i].h,
				recovered_tm[i].i, recovered_tm[i].s);
		} else {
			test_pass = 0;
			printf("[FAIL] test %2d -- Expected %04d-%02d-%02d "
				"%02d:%02d:%02d, but got %04d-%02d-%02d "
				"%02d:%02d:%02d\n", i + 1,
				recovered_tm[i].y, recovered_tm[i].m,
				recovered_tm[i].d, recovered_tm[i].h,
				recovered_tm[i].i, recovered_tm[i].s,
				current_recover.y, current_recover.m,
				current_recover.d, current_recover.h,
				current_recover.i, current_recover.s);
		}
	}
	puts("end    test_decode.");
	return test_pass;
}

static char test_telegram_identity()
{
	const struct dcf77_timelayer_tm tm_in[] = {
		{ .y = 2002, .m = 2, .d = 21, .h = 16, .i = 1, .s = 0 },
	};
	const unsigned char cmp[][DCF77_SECONDLAYER_LINE_BYTES] = {
		{ 0x55,0x55,0x55,0x55,0x55,0xad,0xaa,0xf9,0x6e,0xab,0x5e,0xb9,0xea,0xaa,0x5a, },
	};
	char test_pass = 1;
	unsigned i, j;
	unsigned char current_tel[DCF77_SECONDLAYER_LINE_BYTES];
	puts("begin  test_telegram_identity...");
	for(i = 0; i < sizeof(tm_in)/sizeof(struct dcf77_timelayer_tm); i++) {
		dcf77_timelayer_tm_to_telegram(&tm_in[i], current_tel);
		if(memcmp(current_tel, cmp[i],
					DCF77_SECONDLAYER_LINE_BYTES) == 0) {
			printf("[ OK ] identity %d\n", i);
		} else {
			test_pass = 0;
			printf("[FAIL] identity %d -- Expected ", i);
			for(j = 0; j < DCF77_SECONDLAYER_LINE_BYTES; j++)
				printf("%02x,", cmp[i][j]);
			printf(", but got ");
			for(j = 0; j < DCF77_SECONDLAYER_LINE_BYTES; j++)
				printf("%02x,", current_tel[j]);
			printf("\n");
		}
	}
	puts("end    test_telegram_identity.");
	return test_pass;
}

/* -- begin debug aux procedures, cf test_auto_secondlayer.c -- */
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
	printf("               ");
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
		printf("       %s meml%d=", i == ctx->private_line_current?
								"*": " ", i);
		printtel_sub(ctx->private_telegram_data +
					(i * DCF77_SECONDLAYER_LINE_BYTES));
	}
	printf("       line_current=%u, cursor=%u\n",
			ctx->private_line_current, ctx->private_line_cursor);
}

static void debug_date_mis(const struct dcf77_timelayer_tm* expected,
					struct dcf77_secondlayer* secondlayer,
					struct dcf77_timelayer* timelayer)
{
	printf("       Datetime mismatch. Expected %02d.%02d.%04d "
		"%02d:%02d:%02d, got %02d.%02d.%04d %02d:%02d:%02d\n",
		expected->d, expected->m, expected->y,
		expected->h, expected->i, expected->s,
		timelayer->out_current.d, timelayer->out_current.m,
		timelayer->out_current.y, timelayer->out_current.h,
		timelayer->out_current.i, timelayer->out_current.s);
	dumpmem(secondlayer);
}
/* -- end debug aux procedures -- */

/*
 * QOS test is intended to test the degradation upon receiving bad signals.
 */
static char test_quality_of_service()
{
	char test_pass = 1;
	#define MAX_SIGNALS_PER_TEST 1024
	#define MAX_CHECKPOINTS_PER_TEST 32
	const char* test_descr[] = {
		/* test 0 */
		"13.04.2019 17:00:00-17:01:01 undisturbed test at EOM",
		/* test 1 */
		"13.04.2019 17:00:00-17:01:01 go down to QOS9 for invalid data",
		/* test 2 */
		"22.04.2019 22:41:00-22:42:01 with only tens matching / QOS2",
		/* test 3 */
		"22.04.2019 22:41:00-22:43:00 with recovery from QOS2->QOS1",
		/* test 4 */
		"22.04.2019 22:41:00-22:42:01 with tens unchanged QOS2",
		/* test 5 */
		"13.04.2019 17:29:00-17:30:00 tens change undisturbed QOS1",
		/* test 6 */
		"13.04.2019 17:29:00-17:30:00 tens change prev recovery QOS3",
		/* test 7 */
		"22.04.2019 22:41:00-22:52:00 long clear signal QOS1",
		/* test 8 */
		/* TODO CSTAT SMALL PROBLEM: THIS TEST SHOWS AN ERROR: IT INTERMEDIATELY OUTPUTS 22:40:00 instead of 22:50:00. It can be explained in part by aggressive xeliminate which fills the missing minute tens from the previous minutes' data. However, this does not explain why a telegram with invalid minute checksum is generated. Such a telegram should never be output and instead the result be doubted before passing to timelayer. Finally, I suspect there are cases where the timelayer cannot detect such cases (e.g. checksum received as 3/_ rather than definitive 0). In these cases it should still not permit the clock to jump from 22:49:00 to 22:40:00. It might make sense to degrade to sort of QOS9 and jump back as soon as the data is definitive. / Should focus on how that "definitiveness" can be sharpened.
		"22.04.2019 22:41:00-22:52:00 TODO TEST QOS4",
		/* TODO TEST QOS4, QOS5, QOS6, QOS7, QOS8, QOS9 */
	};
	const unsigned num_signals[] = {
		/* test 0 */
		121,
		/* test 1 */
		121,
		/* test 2 */
		121,
		/* test 3 */
		180,
		/* test 4 */
		121,
		/* test 5 */
		120,
		/* test 6 */
		120,
		/* test 7 */
		720,
		/* test 8 */
		720,
	};
	const enum dcf77_bitlayer_reading signals[][MAX_SIGNALS_PER_TEST] = {
		/* test 0 */
		{
			/* 13.04.2019 17:00:00 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 3, 2, 3, 2, 2, 2,
			2, 2, 2, 2, 2, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 13.04.2019 17:01:00 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 2, 2, 3, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 13.04.2019 17:01:01 */
			2,
		},
		/* test 1 */
		{
			/* 13.04.2019 17:00:00 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 3, 2, 3, 2, 2, 2,
			2, 2, 2, 2, 2, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 13.04.2019 17:01:00 | here illegal begin of min=1 */
			3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2,
			2, 2, 2, 2, 3, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 13.04.2019 17:01:01 */
			2,
		},
		/* test 2 */
		{
			/* 22.04.2019 22:41:00 */
			2, 2, 2, 2, 3, 2, 2, 2, 3, 2, 3, 3,
			2, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 2,
			2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 22.04.2019 22:4_:00 */
			2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2,
			2, 3, 2, 2, 2, 3, 2, 2, 3, 1, 1, 1,
			1, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 12.04.2019 22:42:01 */
			2,
		},
		/* test 3 */
		{
			/* 22.04.2019 22:41:00 */
			2, 2, 2, 2, 3, 2, 2, 2, 3, 2, 3, 3,
			2, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 2,
			2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 22.04.2019 22:4_:00 */
			2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2,
			2, 3, 2, 2, 2, 3, 2, 2, 3, 1, 1, 1,
			1, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 12.04.2019 22:42:01 */
			2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 3,
			3, 2, 3, 2, 2, 3, 2, 2, 3, 3, 3, 2,
			2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
		},
		/* test 4 */
		{
			/* 22.04.2019 22:41:00 */
			2, 2, 2, 2, 3, 2, 2, 2, 3, 2, 3, 3,
			2, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 2,
			2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 22.04.2019 22:__:00 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 3, 2, 2, 3, 1, 1, 1,
			1, 1, 1, 1, 1, 2, 3, 2, 2, 2, 3, 2,
			2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 12.04.2019 22:42:01 */
			2,
		},
		/* test 5 */
		{
			/* 13.04.2019 17:29:00 */
			2, 3, 2, 3, 3, 3, 2, 3, 3, 2, 3, 2,
			3, 3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2,
			3, 2, 3, 2, 3, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 13.04.2019 17:30:00 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2,
			2, 3, 3, 2, 2, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
		},
		/* test 6 */
		{
			/* 13.04.2019 17:29:00 */
			2, 3, 2, 3, 3, 3, 2, 3, 3, 2, 3, 2,
			3, 3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2,
			3, 2, 3, 2, 3, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 2, 2, 3,
			2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
			/* 13.04.2019 17:__:00 */
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2,
			2, 3, 3, 2, 2, 3, 3, 3, 2, 3, 2, 2,
			3, 3, 2, 2, 3, 2, 2, 3, 3, 1, 1, 1,
			1, 1, 3, 2, 2, 3, 3, 2, 2, 2, 3, 1,
		},
		/* test 7 */
		{
			/* 22.04.2019 22:41:00 */
			2,2,2,2,3,2,2,2,3,2,3,3,2,2,2,2,2,3,2,2,
			3,3,2,2,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:42:00 */
			2,3,2,3,3,3,2,3,3,3,2,2,2,3,2,2,2,3,2,2,
			3,2,3,2,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:43:00 */
			2,2,3,3,2,2,2,2,2,2,2,3,3,2,3,2,2,3,2,2,
			3,3,3,2,2,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:44:00 */
			2,2,3,2,2,2,3,3,2,2,2,2,2,3,2,2,2,3,2,2,
			3,2,2,3,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:45:00 */
			2,2,2,3,2,3,3,3,3,3,3,3,2,2,3,2,2,3,2,2,
			3,3,2,3,2,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:46:00 */
			2,2,3,2,3,2,3,3,2,2,3,2,3,3,3,2,2,3,2,2,
			3,2,3,3,2,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:47:00 */
			2,2,2,2,2,3,3,3,2,2,2,2,2,3,3,2,2,3,2,2,
			3,3,3,3,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:48:00 */
			2,3,3,2,2,2,3,3,3,3,2,2,2,3,3,2,2,3,2,2,
			3,2,2,2,3,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:49:00 */
			2,2,3,3,3,2,2,3,2,3,2,3,2,3,2,2,2,3,2,2,
			3,3,2,2,3,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:50:00 */
			2,3,3,2,3,3,2,3,3,3,2,3,3,3,3,2,2,3,2,2,
			3,2,2,2,2,3,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:51:00 */
			2,3,2,2,2,2,3,3,3,3,3,2,2,2,2,2,2,3,2,2,
			3,3,2,2,2,3,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:52:00 */
			2,2,2,3,3,2,3,3,2,3,3,2,2,2,3,2,2,3,2,2,
			3,2,3,2,2,3,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
		},
		/* test 8 */
		{
			/* 22.04.2019 22:41:00 */
			2,2,2,2,3,2,2,2,3,2,3,3,2,2,2,2,2,3,2,2,
			/* 1: minute tens  end*/
			3,3,2,2,2,2,2,1,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:42:00 */
			2,3,2,3,3,3,2,3,3,3,2,2,2,3,2,2,2,3,2,2,
			3,2,3,2,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:43:00 */
			2,2,3,3,2,2,2,2,2,2,2,3,3,2,3,2,2,3,2,2,
			3,3,3,2,2,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:44:00 */
			2,2,3,2,2,2,3,3,2,2,2,2,2,3,2,2,2,3,2,2,
			3,2,2,3,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:45:00 */
			2,2,2,3,2,3,3,3,3,3,3,3,2,2,3,2,2,3,2,2,
			3,3,2,3,2,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:46:00 */
			2,2,3,2,3,2,3,3,2,2,3,2,3,3,3,2,2,3,2,2,
			3,2,3,3,2,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:47:00 */
			2,2,2,2,2,3,3,3,2,2,2,2,2,3,3,2,2,3,2,2,
			3,3,3,3,2,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:48:00 */
			2,3,3,2,2,2,3,3,3,3,2,2,2,3,3,2,2,3,2,2,
			3,2,2,2,3,2,2,3,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:49:00 */
			2,2,3,3,3,2,2,3,2,3,2,3,2,3,2,2,2,3,2,2,
			3,3,2,2,3,2,2,3,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:50:00 */
			2,3,3,2,3,3,2,3,3,3,2,3,3,3,3,2,2,3,2,2,
			3,2,2,2,2,1,1,1,2,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:51:00 */
			2,3,2,2,2,2,3,3,3,3,3,2,2,2,2,2,2,3,2,2,
			3,3,2,2,2,1,1,1,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
			/* 22.04.2019 22:52:00 */
			2,2,2,3,3,2,3,3,2,3,3,2,2,2,3,2,2,3,2,2,
			3,2,3,2,2,1,1,1,3,2,3,2,2,2,3,2,2,3,2,2,
			2,3,3,2,2,2,2,3,2,2,3,2,2,3,3,2,2,2,3,1,
		},
	};
	const unsigned checkpoints_loc[][MAX_CHECKPOINTS_PER_TEST] = {
		/* test 0 */
		{
			59,
			119,
			120,
			1024,
		},
		/* test 1 */
		{
			59,
			119,
			120,
			1024,
		},
		/* test 2 */
		{
			59,
			119,
			120,
			1024,
		},
		/* test 3 */
		{
			59,
			119,
			120,
			179,
			1024,
		},
		/* test 4 */
		{
			59,
			119,
			120,
			1024,
		},
		/* test 5 */
		{
			59,
			119,
			1024,
		},
		/* test 6 */
		{
			59,
			119,
			1024,
		},
		/* test 7 */
		{
			 59,
			119,
			179,
			239,
			299,
			359,
			419,
			479,
			539,
			599,
			659,
			719,
			1024,
		},
		/* test 8 */
		{
			 59,
			119,
			179,
			239,
			299,
			359,
			419,
			479,
			539,
			599,
			659,
			719,
			1024,
		},
	};
	const struct dcf77_timelayer_tm checkpoints_val[][
						MAX_CHECKPOINTS_PER_TEST] = {
		/* test 0 */
		{
			{ .y = 2019, .m = 4, .d = 13, .h = 17, .i = 0, .s = 0 },
			{ .y = 2019, .m = 4, .d = 13, .h = 17, .i = 1, .s = 0 },
			{ .y = 2019, .m = 4, .d = 13, .h = 17, .i = 1, .s = 1 },
		},
		/* test 1 */
		{
			{ .y = 2019, .m = 4, .d = 13, .h = 17, .i = 0, .s = 0 },
			{ .y = 2019, .m = 4, .d = 13, .h = 17, .i = 1, .s = 0 },
			{ .y = 2019, .m = 4, .d = 13, .h = 17, .i = 1, .s = 1 },
		},
		/* test 2 */
		{
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 41,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 1 },
		},
		/* test 3 */
		{
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 41,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 1 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 43,.s = 0 },
		},
		/* test 4 */
		{
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 41,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 1 },
		},
		/* test 5 */
		{
			{ .y = 2019,.m = 4,.d = 13,.h = 17,.i = 29,.s = 0 },
			{ .y = 2019,.m = 4,.d = 13,.h = 17,.i = 30,.s = 0 },
		},
		/* test 6 */
		{
			{ .y = 2019,.m = 4,.d = 13,.h = 17,.i = 29,.s = 0 },
			{ .y = 2019,.m = 4,.d = 13,.h = 17,.i = 30,.s = 0 },
		},
		/* test 7 */
		{
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 41,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 43,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 44,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 45,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 46,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 47,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 48,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 49,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 50,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 51,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 52,.s = 0 },
		},
		/* test 8 */
		{
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 41,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 42,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 43,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 44,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 45,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 46,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 47,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 48,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 49,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 50,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 51,.s = 0 },
			{ .y = 2019,.m = 4,.d = 22,.h = 22,.i = 52,.s = 0 },
		},
	};
	enum dcf77_timelayer_qos checkpoints_qos[][MAX_CHECKPOINTS_PER_TEST] = {
		/* test 0 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
		},
		/* test 1 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS9_ASYNC,
			DCF77_TIMELAYER_QOS9_ASYNC,
		},
		/* test 2 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS2,
			DCF77_TIMELAYER_QOS2,
		},
		/* test 3 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS2,
			DCF77_TIMELAYER_QOS2,
			DCF77_TIMELAYER_QOS1,
		},
		/* test 4 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS2,
			DCF77_TIMELAYER_QOS2,
		},
		/* test 5 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
		},
		/* test 6 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS3,
		},
		/* test 7 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
		},
		/* test 8 */
		{
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS1,
			DCF77_TIMELAYER_QOS4,
			DCF77_TIMELAYER_QOS4,
			DCF77_TIMELAYER_QOS4,
		},
	};
	const int num_tests = sizeof(num_signals)/sizeof(unsigned);
	unsigned i;
	unsigned j;
	unsigned chk;

	struct dcf77_secondlayer secondlayer;
	struct dcf77_timelayer timelayer;

	puts("begin  test_quality_of_service...");
	for(i = 0; i < num_tests; i++) {
		chk = 0;

		dcf77_secondlayer_init(&secondlayer);
		dcf77_timelayer_init(&timelayer);
		for(j = 0; j < num_signals[i]; j++) {
			secondlayer.in_val = signals[i][j];
			dcf77_secondlayer_process(&secondlayer);
			dcf77_timelayer_process(&timelayer, 1, &secondlayer);
			if(j == checkpoints_loc[i][chk]) {
				/* now perform a check */
				if(memcmp(&checkpoints_val[i][chk],
				&timelayer.out_current,
				sizeof(struct dcf77_timelayer_tm)) != 0) {
					debug_date_mis(&checkpoints_val[i][chk],
							&secondlayer,
							&timelayer);
					printf("       out_telegram_1[%2d] = ",
						secondlayer.out_telegram_1_len);
					printtel_sub(
						secondlayer.out_telegram_1);
					printf("       out_telegram_2[%2d] = ",
						secondlayer.out_telegram_2_len);
					printtel_sub(
						secondlayer.out_telegram_2);
					printf("       qos = %d\n",
							timelayer.out_qos);
					test_pass = 0;
					break;
				}
				if(checkpoints_qos[i][chk]
							!= timelayer.out_qos) {
					printf("       QOS mismatch. Expected "
						"%d, got %d at %d\n",
						checkpoints_qos[i][chk],
						timelayer.out_qos, j);
					test_pass = 0;
					break;
				}
				chk++;
			}
			secondlayer.out_telegram_1_len = 0; /* TODO DEBUG ONLY? */
		}

		if(j != num_signals[i]) {
			printf("[FAIL] %s\n", test_descr[i]);
		} else {
			printf("[ OK ] %s\n", test_descr[i]);
		}
	}
	puts("end    test_quality_of_service.");
	return test_pass;
}
