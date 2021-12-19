#include "../src/dcf77_timelayer.c"

/*
 * "Idenitity Test" for encode and decode of telegram.
 * This verifies that every valid struct dcf77_timelayer_tm can be encoded to
 * a telegram and the resulting telegram deocdes back to the same tm.
 *
 * USAGE:
 * cbmc --trace -I ../interface test_telegram_identity.c
 *
 * VERIFICATION SUCCESSFUL as per 2021-12-19
 */
void main()
{
	struct dcf77_timelayer_tm tm_under_test; /* nondeterministic */

	struct dcf77_timelayer_tm tm_intermediate;
	unsigned char tel_intermediate[DCF77_SECONDLAYER_LINE_BYTES];

	/*
	 * Limit search space to valid dates assuming date of compilation in
	 * range 2000..2099
	 */
	__CPROVER_assume(
		tm_under_test.y <= 2099 &&
		tm_under_test.y >= 2000 &&
		tm_under_test.m >= 1 && tm_under_test.m <= 12 &&
		tm_under_test.h < 24 &&
		tm_under_test.i < 60 &&
		tm_under_test.s == 0
	);
	if(tm_under_test.m == 2 &&
				dcf77_timelayer_is_leap_year(tm_under_test.y))
		__CPROVER_assume(tm_under_test.d <= MONTH_LENGTHS[0]);
	else
		__CPROVER_assume(tm_under_test.d <=
						MONTH_LENGTHS[tm_under_test.m]);
	
	dcf77_timelayer_tm_to_telegram(&tm_under_test, tel_intermediate);
	dcf77_timelayer_decode(&tm_intermediate, tel_intermediate);

	__CPROVER_assert(tm_under_test.y == tm_intermediate.y, "Years match");
	__CPROVER_assert(tm_under_test.m == tm_intermediate.m, "Months match");
	__CPROVER_assert(tm_under_test.d == tm_intermediate.d, "Days match");
	__CPROVER_assert(tm_under_test.h == tm_intermediate.h, "Hours match");
	__CPROVER_assert(tm_under_test.i == tm_intermediate.i, "Minutes match");
}
