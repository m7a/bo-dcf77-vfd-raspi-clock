/* depends dcf77_secondlayer.h */

/*
 * Leapsec EOMs are not allowed! NO_UPDATE contents are not allowed!
 *
 * Returns 1 if telegram is correct/OK.
 * Returns 0 if telegram is incorrect/WRONG.
 */
char dcf77_secondlayer_check_bcd_correct_telegram(
		struct dcf77_secondlayer* ctx, unsigned char tel_start_line,
		unsigned char tel_start_offset_in_line);

/*
 * Variant that allows Leapsec EOMs by not checking the EOM at all.
 */
char dcf77_secondlayer_check_bcd_correct_telegram_ignore_eom(
		struct dcf77_secondlayer* ctx, unsigned char tel_start_line,
		unsigned char tel_start_offset_in_line);
