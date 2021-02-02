#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_offsets.h"
#include "dcf77_line.h"

struct check_state {
	struct dcf77_secondlayer* ctx;
	unsigned char tel_start_line;
	unsigned char* next_line_ptr;
	char is_next_empty;
};

enum parity_state {
	PARITY_SUM_EVEN_PASS = 0,
	PARITY_SUM_ODD_MISM  = 1,
	PARITY_SUM_UNDEFINED = 2,
};

static unsigned char to_bcd(unsigned char by);
static unsigned char read_multiple(struct check_state* s,
				unsigned char bit_offset, unsigned char length);
static unsigned char read_byte(struct check_state* s, unsigned char bit_offset);
static void update_parity(enum parity_state* par, unsigned char by);

/* TODO CSTAT WRITE A TEST FOR THIS AND TEST THIS PROCEDURE THOROUGHLY! */

/*
 * Leapsec EOMs are not allowed!
 *
 * Returns 1 if telegram is correct/OK.
 * Returns 0 if telegram is incorrect/WRONG.
 */
char dcf77_secondlayer_check_bcd_correct_telegram(struct dcf77_secondlayer* ctx,
				unsigned char tel_start_line,
				unsigned char tel_start_offset_in_line)
{
	unsigned char* next_line_ptr = dcf77_line_pointer(ctx,
					dcf77_line_next(tel_start_line));
	struct check_state s = {
		.ctx            = ctx,
		.tel_start_line = tel_start_line,
		.next_line_ptr  = next_line_ptr,
		.is_next_empty  = dcf77_line_is_empty(next_line_ptr),
	};

	enum parity_state parity_minute = PARITY_SUM_EVEN_PASS;
	enum parity_state parity_hour   = PARITY_SUM_EVEN_PASS;
	enum parity_state parity_whole  = PARITY_SUM_EVEN_PASS;

	unsigned char entry;

	/*     0 -- begin of minute is constant 0 */
	if(read_multiple(&s, tel_start_offset_in_line + 0, 1) == DCF77_BIT_1)
		return 0;

	/*
	 * 17-18 -- CET (01) / CEST (10) => 11 invalid, 00 invalid
	 *          11 is 00 00 11 11 = 0x0f; 00 is 00 00 10 10 = 0x0a
	 */
	switch(read_multiple(&s, tel_start_offset_in_line +
				DCF77_OFFSET_DAYLIGHT_SAVING_TIME, 2)) {
	case 0x0a:
	case 0x0f:
		return 0;
	}

	/*    20 -- Begin Time needs to be constant 1 */
	if(read_multiple(&s, tel_start_offset_in_line +
				DCF77_OFFSET_BEGIN_TIME, 1) == DCF77_BIT_0)
		return 0;

	/* 21-24 -- minute ones range from 0..9 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_MINUTE_ONES, 4);
	if(to_bcd(entry) > 9)
		return 0;

	update_parity(&parity_minute, entry);
	update_parity(&parity_whole,  entry);

	/* 25-27 -- minute tens range from 0..6 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_MINUTE_TENS, 3);
	if(to_bcd(entry) > 6)
		return 0;

	update_parity(&parity_minute, entry);
	update_parity(&parity_whole,  entry);

	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_PARITY_MINUTE, 1);
	update_parity(&parity_minute, entry);
	update_parity(&parity_whole,  entry);

	/* telegram failing minute parity is invalid */
	if(parity_minute == PARITY_SUM_ODD_MISM)
		return 0;

	/* 29-32 -- hour ones range from 0..9 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_HOUR_ONES, 4);
	if(to_bcd(entry) > 9)
		return 0;

	update_parity(&parity_hour,  entry);
	update_parity(&parity_whole, entry);

	/* 33-34 -- hour tens range from 0..2 (anything but 11 = 3 is valid) */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_HOUR_TENS, 2);
	if(entry == 0x0f)
		return 0;

	update_parity(&parity_hour,  entry);
	update_parity(&parity_whole, entry);

	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_PARITY_HOUR, 1);
	update_parity(&parity_hour,  entry);
	update_parity(&parity_whole, entry);

	/* telegram failing hour parity is invalid */
	if(parity_hour == PARITY_SUM_ODD_MISM)
		return 0;

	/* 36-39 -- day ones ranges from 0..9 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_DAY_ONES, 4);
	if(to_bcd(entry) > 9)
		return 0;

	update_parity(&parity_whole, entry);
	
	/* 40-41 -- all day tens are valid (0..3) -- nothing to check here */
	update_parity(&parity_whole, read_multiple(&s,
			tel_start_offset_in_line + DCF77_OFFSET_DAY_TENS, 1));

	/*
	 * 42-44 -- all day of week (1-7) are valid except "0"
	 *          0x2a = 00 10 10 10 = 3x"0"
	 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_DAY_OF_WEEK, 3);
	if(entry == 0x2a)
		return 0;

	update_parity(&parity_whole, entry);

	/* 49    -- month tens are all valid (0..1) -- nothing to check */
	update_parity(&parity_whole, read_multiple(&s,
			tel_start_offset_in_line + DCF77_OFFSET_MONTH_TENS, 1));

	/* 50-53 -- year ones are valid from 0..9 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_YEAR_ONES, 4);
	if(to_bcd(entry) > 9)
		return 0;

	update_parity(&parity_whole, entry);

	/* 54-57 -- year tens are valid from 0..9 */
	entry = read_multiple(&s, tel_start_offset_in_line +
						DCF77_OFFSET_YEAR_TENS, 4);
	if(to_bcd(entry) > 9)
		return 0;

	update_parity(&parity_whole, entry);

	update_parity(&parity_whole, read_multiple(&s,
		tel_start_offset_in_line + DCF77_OFFSET_PARITY_DATE, 1));

	/* date parity failed */
	if(parity_whole == PARITY_SUM_ODD_MISM)
		return 0;

	/*    59 -- End of minute (only regular ones accepted) */
	switch(read_multiple(&s, tel_start_offset_in_line +
					DCF77_OFFSET_ENDMARKER_REGULAR, 1)) {
	case DCF77_BIT_0:
	case DCF77_BIT_1:
		return 0;
	}

	/* nothing found to be incorrect => accept */
	return 1;
}

/*
 * Takes up to four "bit" of internally represented data and transforms them to
 * unsinged C byte where 0 is BCD-0, 1 is BCD-1 etc.
 *
 * "&" adjacnet bits returns 1 exactly if it is DCF77_BIT_1.
 * move the obtained ones to the lower output parts s.t. output values range
 * from 0-15.
 */
static unsigned char to_bcd(unsigned char by)
{
	return  (((by & 0x01) & (by & 0x02) >> 1) >> 0) |
		(((by & 0x04) & (by & 0x08) >> 1) >> 1) |
		(((by & 0x10) & (by & 0x20) >> 1) >> 2) |
		(((by & 0x40) & (by & 0x80) >> 1) >> 3);
}

static unsigned char read_multiple(struct check_state* s,
				unsigned char bit_offset, unsigned char length)
{
	unsigned char upper_low = read_byte(s, bit_offset);
	unsigned char lower_up  = read_byte(s, bit_offset + length - 1);
	unsigned char shf_ulow  =     2 * (bit_offset % 4);
	unsigned char shf_loup  = 8 - 2 * (bit_offset % 4);
	unsigned char shfl      = 8 - 2 * length;
	return (((upper_low & (0xff << shf_ulow)) >> shf_ulow) |
		((lower_up  & (0xff >> shf_loup)) << shf_loup)) &
		(0xff >> shfl);
}

static unsigned char read_byte(struct check_state* s, unsigned char bit_offset)
{
	unsigned char byte_offset = bit_offset / 4;
	if(byte_offset < DCF77_SECONDLAYER_LINE_BYTES)
		return dcf77_line_pointer(s->ctx,
						s->tel_start_line)[byte_offset];
	else if(s->is_next_empty)
		return 0xaa; /* all set to no_signal */
	else
		return s->next_line_ptr[byte_offset];
}

static void update_parity(enum parity_state* par, unsigned char by)
{
	/*
	 * requires 0 = DCF77_BI_NO_SIGNAL that does not appear in regular data
	 * except ot mark "end"s
	 */
	while((by != 0) && (*par != PARITY_SUM_UNDEFINED)) {
		switch(by & 0x03) {
		case DCF77_BIT_1:         *par = !*par;                break;
		case DCF77_BIT_NO_SIGNAL: *par = PARITY_SUM_UNDEFINED; break;
		}
		by >>= 2;
	}
}
