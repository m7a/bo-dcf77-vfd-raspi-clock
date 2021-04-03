#include "../src/dcf77_secondlayer_check_bcd_correct_telegram.c"

/*
 * "Disruption Test" for Check Telegram Procedure
 *
 * USAGE:
 * cbmc --trace --unwindset update_parity.0:4 -I ../interface test_checkbcd_disr.c
 *
 * VERIFICATION SUCCESSFUL 2021/03/04 19:25:53 CEST
 */

void main()
{
	/* nondeterminstic disruption locations */
	unsigned char disruptlocs[60];

	unsigned char i = 0;

	unsigned char firsttel[DCF77_SECONDLAYER_LINE_BYTES];
	unsigned char secondtel[DCF77_SECONDLAYER_LINE_BYTES];

	/* nondeterministic context (only first telegram left nondet) */
	struct dcf77_secondlayer ctx;

	/* initialize second line onwards to empty */
	for(i = DCF77_SECONDLAYER_LINE_BYTES; i < DCF77_SECONDLAYER_MEM; i++)
		ctx.private_telegram_data[i] = DCF77_BIT_NO_UPDATE;

	/* initialize unused parts to 0/empty etc. */
	ctx.private_inmode = IN_BACKWARD;
	ctx.private_line_current = 0;
	ctx.private_line_cursor = 0;
	ctx.private_leap_in_line = 0;
	ctx.private_leap_second_expected = 0;
	ctx.in_val = 0;
	ctx.out_telegram_1_len = 0;
	ctx.out_telegram_2_len = 0;
	for(i = 0; i < DCF77_SECONDLAYER_LINE_BYTES; i++) {
		ctx.out_telegram_1[i] = 0x00;
		ctx.out_telegram_2[i] = 0x00;
	}
	ctx.fault_reset = 0;

	for(i = 0; i < DCF77_SECONDLAYER_LINE_BYTES; i++)
		firsttel[i] = ctx.private_telegram_data[i];

	/* precondition: Nondeterministically generated telegram is valid */
	if(dcf77_secondlayer_check_bcd_correct_telegram(&ctx, 0, 0)) {

		/* now disrupt any number of bytes (at least one) */
		for(i = 0; i < 60; i++) {
			__CPROVER_assume(disruptlocs[i] < 60);

			dcf77_telegram_write_bit(disruptlocs[i],
						&ctx.private_telegram_data,
						DCF77_BIT_NO_SIGNAL);
		}

		/* store interesting data */
		for(i = 0; i < DCF77_SECONDLAYER_LINE_BYTES; i++)
			secondtel[i] = ctx.private_telegram_data[i];

		/* now assert that the telegram remains valid */
		assert(dcf77_secondlayer_check_bcd_correct_telegram(&ctx,0,0));
	}
}
