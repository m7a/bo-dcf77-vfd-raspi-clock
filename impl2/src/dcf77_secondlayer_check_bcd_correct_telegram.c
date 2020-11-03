#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_offsets.h"

struct dcf77_secondlayer_check_bcd_correct_telegram_state {
	struct dcf77_secondlayer* ctx;
	unsigned char tel_start_line;
	unsigned char* next_line_ptr;
	char is_next_empty;
};

/*
 * Leapsec EOMs are not allowed!
 *
 * Returns 1 if telegram is correct/OK.
 * Returns 0 if telegram is incorrect/WRONG.
 */
char dcf77_secondlayer_check_bcd_correct_telegram(struct dcf77_secondlayer* ctx,
				unsigned char tel_start_line,
				unsigned char tel_start_offset_in_line) {

	unsigned char* next_line_ptr = dcf77_line_pointer(ctx,
					dcf77_line_next(tel_start_line));

	struct dcf77_secondlayer_check_bcd_correct_telegram_state s = {
		.ctx            = ctx,
		.tel_start_line = tel_start_line,
		.next_line_ptr  = next_line_ptr,
		.is_next_empty  = dcf77_line_is_empty(next_line);
	};

	unsigned char entry;

	/*     0 -- begin of minute is constant 0 */
	if(dcf77_secondlayer_check_bcd_correct_telegram_read_bit_pwrap(&s,
				tel_start_offset_in_line + 0) == DCF77_BIT_1)
		return 0;

	/*
	 * 17-18 -- CET (01) / CEST (10) => 11 invalid, 00 invalid
	 *          11 is 00 00 11 11 = 0x0f; 00 is 00 00 10 10 = 0x0a
	 */
	entry = read_multiple(&s, tel_start_offset_in_line +
					DCF77_OFFSET_DAYLIGHT_SAVING_TIME, 2);
	if( ((entry & 0x0f) == 0x0f) || ((entry & 0x0a) == 0x0a) )
		return 0;

	

}

static unsigned char dcf77_secondlayer_check_bcd_correct_telegram_read_bit_pwrap
		(struct dcf77_secondlayer_check_bcd_correct_telegram_state* s,
		unsigned char bitidx)
{
	if(bitidx < 60) {
		/* regular read */
		return dcf77_telegram_read_bit(bitidx, dcf77_line_pointer(
						s->ctx, s->tel_start_line));
	} else {
 		/*
		 * reads into next telegram
		 * Here, empty lines are not skipped because an empty area in
		 * the buffer indicates that wraparound will lead from last to
		 * first telegram between which we do not move anyways.
		 */
		bitidx -= 60;
		return s->is_next_empty? DCF77_BIT_NO_UPDATE:
			dcf77_telegram_read_bit(bitidx, s->next_line_ptr);
	}
}

/* TODO CSTAT BELOW IS NOT PROPERLY IMPLEMENTED YET. IT ALSO NEEDS TO TAKE INTO CONSIDERATION THE RINGBUFFER SEMANTICS! */

unsigned char dcf77_secondlayer_check_bcd_correct_telegram_read_multiple(
		struct dcf77_secondlayer_check_bcd_correct_telegram_state* s,
		unsigned char bitidx, unsigned char length)
{
	unsigned char lower;
	unsigned char bit = bitidx % 4;
	unsigned char shf = 8 - 2 * bit;

	/* lower */
	if(bitidx < 60) {
		/* first part is regular read */
		lower = ((s->ctx->private_line_data[s->tel_start_line *
				DCF77_SECONDLAYER_LINE_BYTES + (bitidx / 4)] &
				(0xff >> shf)) << shf);
	} else if(s->is_next_empty) {
		lower = 0;
	} else {
		/* first part reads from next line if not empty */
		bitidx -= 60;
		lower = ((s->ctx->next_line_ptr[bitidx / 4] & (0xff >> shf))
									<< shf);
	}

	/* upper */
	/* TODO CSTAT CHECK IF "lower" part above is correct then add "upper" part here... */
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
