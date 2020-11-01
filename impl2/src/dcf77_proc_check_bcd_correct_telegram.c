#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_offsets.h"

/*
 * Leapsec EOMs are not allowed!
 *
 * Returns 1 if telegram is correct/OK.
 * Returns 0 if telegram is incorrect/WRONG.
 */
char dcf77_proc_check_bcd_correct_telegram(struct dcf77_secondlayer* ctx,
				unsigned char tel_start_line,
				unsigned char tel_start_offset_in_line) {
	unsigned char entry;
	unsigned char telsl = ctx->private_line_lengths[tel_start_line];
	unsigned char byoff = tel_start_line * DCF77_SECONDLAYER_LINE_BYTES;

	/*     0 -- begin of minute is constant 0 */
	if(dcf77_telegram_read_bit(ctx->private_telegram_data, byoff,
			tel_start_offset_in_line /* + 0 */) == DCF77_BIT_1)
		return 0;

	/*
	 * 17-18 -- CET (01) / CEST (10) => 11 invalid, 00 invalid
	 *          11 is 00 00 11 11 = 0x0f; 00 is 00 00 10 10 = 0x0a
	 */
	entry = read_multiple(ctx, telsl, byoff, tel_start_offset_in_line,
					DCF77_OFFSET_DAYLIGHT_SAVING_TIME, 2);
	if( ((entry & 0x0f) == 0x0f) || ((entry & 0x0a) == 0x0a) )
		return 0;

	

}

/* TODO CSTAT BELOW IS NOT PROPERLY IMPLEMENTED YET. IT ALSO NEEDS TO TAKE INTO CONSIDERATION THE RINGBUFFER SEMANTICS! */

unsigned char read_bit_pwrap(
	struct dcf77_secondlayer* ctx, unsigned char tel_start_line_length,
	unsigned char byoff, unsigned char tel_start_offset_in_line,
	unsigned char offset_in_tel)
{

	if(tel_start_offset_in_line + offset_in_tel >= tel_start_line_length)
		byoff++; /* skip byte with at idx 15 */

	return dcf77_telegram_read_bit(ctx->private_telegram_data +
							byoff, offset_in_line);
}

unsigned char read_multiple(
	struct dcf77_secondlayer* ctx, unsigned char tel_start_line_length,
	unsigned char byoff, unsigned char tel_start_offset_in_line,
	unsigned char offset_in_tel, unsigned char num_to_read)
{
	if((tel_start_line_length == 61) && ((tel_start_offset_in_line + offset_in_tel) == 60)) {
		/* read one bit from leap sec marker, then fill from next line */
	} else if((tel_start_line_length == 61) && ((tel_start_offset_in_line + offset_in_tel + num_to_read) >= 60)) {
		/* read from start, then add from leap, then fill from next line */
	} else if(tel_start_offset_in_line + offset_in_tel + num_to_read >= 60) {
		/* read from start, then fill from next line */
	} else {
		/* read from start only */
	}
}
