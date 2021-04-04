#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_offsets.h"
#include "dcf77_line.h"

#include "debugprintf.h"

struct check_state {
	struct dcf77_secondlayer* ctx;
	unsigned char tel_start_line;
	unsigned char tel_start_offset_in_line;
	unsigned char* next_line_ptr;
	char is_next_empty;
};

enum parity_state {
	PARITY_SUM_EVEN_PASS = 0,
	PARITY_SUM_ODD_MISM  = 1,
	PARITY_SUM_UNDEFINED = 2,
};

static char check_begin_of_minute(struct check_state* s);
static unsigned char read_multiple(struct check_state* s,
			unsigned char bit_offset_in_tel, unsigned char length);
static unsigned char read_byte(struct check_state* s, unsigned char bit_offset);
static char check_dst(struct check_state* s);
static char check_begin_of_time(struct check_state* s);
static char check_minute(struct check_state* s);
static unsigned char to_bcd(unsigned char by);
static void update_parity(enum parity_state* par, unsigned char by);
static char check_hour(struct check_state* s);
static char check_date(struct check_state* s);
static char check_end_of_minute(struct check_state* s);

/*
 * Leapsec EOMs are not allowed! NO_UPDATE contents are not allowed!
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
		.ctx                      = ctx,
		.tel_start_line           = tel_start_line,
		.tel_start_offset_in_line = tel_start_offset_in_line,
		.next_line_ptr            = next_line_ptr,
		.is_next_empty            = dcf77_line_is_empty(next_line_ptr),
	};
	return  check_begin_of_minute(&s) &&
		check_dst(&s)             &&
		check_begin_of_time(&s)   &&
		check_minute(&s)          &&
		check_hour(&s)            &&
		check_date(&s)            &&
		check_end_of_minute(&s);
}

static char check_begin_of_minute(struct check_state* s)
{
	/*     0 -- begin of minute is constant 0 */
	if(read_multiple(s, 0, 1) == DCF77_BIT_1) {
		DEBUGPRINTF("bcdcorrect,ERROR1\n");
		return 0;
	} else {
		return 1;
	}
}

static unsigned char read_multiple(struct check_state* s,
			unsigned char bit_offset_in_tel, unsigned char length)
{
	unsigned char bit_offset = s->tel_start_offset_in_line +
							bit_offset_in_tel;
	unsigned char upper_low  = read_byte(s, bit_offset);
	unsigned char lower_up   = read_byte(s, bit_offset + length - 1);
	unsigned char shf_ulow   =     2 * (bit_offset % 4);
	unsigned char shf_loup   = 8 - 2 * (bit_offset % 4);
	unsigned char shfl       = 8 - 2 * length;
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
		return s->next_line_ptr[byte_offset -
						DCF77_SECONDLAYER_LINE_BYTES];
}


static char check_dst(struct check_state* s)
{
	switch(read_multiple(s, DCF77_OFFSET_DAYLIGHT_SAVING_TIME, 2)) {
	case 0x0a:
	case 0x0f:
		DEBUGPRINTF("bcdcorrect,ERROR2,%02x\n",
			read_multiple(s, DCF77_OFFSET_DAYLIGHT_SAVING_TIME, 2));
		return 0;
	default:
		return 1;
	}
}

static char check_begin_of_time(struct check_state* s)
{

	/*    20 -- Begin Time needs to be constant 1 */
	if(read_multiple(s, DCF77_OFFSET_BEGIN_TIME, 1) == DCF77_BIT_0) {
		DEBUGPRINTF("bcdcorrect,ERROR3\n");
		return 0;
	} else {
		return 1;
	}
}

static char check_minute(struct check_state* s)
{
	unsigned char entry;
	enum parity_state parity_minute = PARITY_SUM_EVEN_PASS;

	/* 21-24 -- minute ones range from 0..9 */
	entry = read_multiple(s, DCF77_OFFSET_MINUTE_ONES, 4);
	if(to_bcd(entry) > 9) {
		DEBUGPRINTF("bcdcorrect,ERROR4,%d\n", to_bcd(entry));
		return 0;
	}
	update_parity(&parity_minute, entry);

	/* 25-27 -- minute tens range from 0..5 */
	entry = read_multiple(s, DCF77_OFFSET_MINUTE_TENS, 3);
	if(to_bcd(entry) > 5) {
		DEBUGPRINTF("bcdcorrect,ERROR5,%d\n", to_bcd(entry));
		return 0;
	}

	update_parity(&parity_minute, entry);

	entry = read_multiple(s, DCF77_OFFSET_PARITY_MINUTE, 1);
	update_parity(&parity_minute, entry);

	/* telegram failing minute parity is invalid */
	if(parity_minute == PARITY_SUM_ODD_MISM) {
		DEBUGPRINTF("bcdcorrect,ERROR6\n");
		return 0;
	}

	return 1;
}

/*
 * Takes up to four "bit" of internally represented data and transforms them to
 * unsinged C byte where 0 is BCD-0, 1 is BCD-1 etc.
 *
 * "&" adjacent bits returns 1 exactly if it is DCF77_BIT_1.
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

static void update_parity(enum parity_state* par, unsigned char by)
{
	/*
	 * requires 0 = DCF77_BIT_NO_SIGNAL that does not appear in regular data
	 * except on mark "end"s
	 */
	while((by != 0) && (*par != PARITY_SUM_UNDEFINED)) {
		switch(by & 0x03) {
		case DCF77_BIT_1:         *par = !*par;                break;
		case DCF77_BIT_NO_SIGNAL: *par = PARITY_SUM_UNDEFINED; break;
		}
		by >>= 2;
	}
}

static char check_hour(struct check_state* s)
{
	unsigned char hour_ones_entry;
	unsigned char hour_ones_bcd;
	unsigned char hour_tens_entry;

	enum parity_state parity_hour = PARITY_SUM_EVEN_PASS;

	/* 29-32 -- hour ones range from 0..9 */
	hour_ones_entry = read_multiple(s, DCF77_OFFSET_HOUR_ONES, 4);
	hour_ones_bcd   = to_bcd(hour_ones_entry);
	if(hour_ones_bcd > 9) {
		DEBUGPRINTF("bcdcorrect,ERROR7,%d\n", hour_ones_bcd);
		return 0;
	}
	update_parity(&parity_hour, hour_ones_entry);

	/* 33-34 -- hour tens range from 0..2 (anything but 11 = 3 is valid) */
	hour_tens_entry = read_multiple(s, DCF77_OFFSET_HOUR_TENS, 2);
	if(hour_tens_entry == 0x0f) {
		DEBUGPRINTF("bcdcorrect,ERROR8\n");
		return 0;
	}
	/* If hour tens is 2 then hour ones may at most be 3 */
	if(hour_tens_entry == 0x0e && hour_ones_bcd > 3) {
		DEBUGPRINTF("bcdcorrect,ERROR8b,%x,%d\n", hour_tens_entry,
								hour_ones_bcd);
		return 0;
	}
	update_parity(&parity_hour, hour_tens_entry);

	update_parity(&parity_hour,
				read_multiple(s, DCF77_OFFSET_PARITY_HOUR, 1));

	/* telegram failing hour parity is invalid */
	if(parity_hour == PARITY_SUM_ODD_MISM) {
		DEBUGPRINTF("bcdcorrect,ERROR9\n");
		return 0;
	}

	return 1;
}

static char check_date(struct check_state* s)
{
	unsigned char day_ones_entry;
	unsigned char month_ones_entry;
	unsigned char entry;
	enum parity_state parity_date = PARITY_SUM_EVEN_PASS;

	/* 36-39 -- day ones ranges from 0..9 */
	day_ones_entry = read_multiple(s, DCF77_OFFSET_DAY_ONES, 4);
	if(to_bcd(day_ones_entry) > 9) {
		DEBUGPRINTF("bcdcorrect,ERROR10,%d\n", to_bcd(day_ones_entry));
		return 0;
	}
	update_parity(&parity_date, day_ones_entry);
	
	/* 40-41 -- all day tens are valid (0..3) */
	entry = read_multiple(s, DCF77_OFFSET_DAY_TENS, 2);
	/*
	 * If day tens is 0 then day ones must not be 0
	 * 0x0a = 00 00 10 10 2x"0" / 0xaa = 10 10 10 10 4x"0"
	 */
	if(entry == 0x0a && day_ones_entry == 0xaa) {
		DEBUGPRINTF("bcdcorrect,ERROR10b\n");
		return 0;
	}
	update_parity(&parity_date, entry);

	/*
	 * 42-44 -- all day of week (1-7) are valid except "0"
	 *          0x2a = 00 10 10 10 = 3x"0"
	 */
	entry = read_multiple(s, DCF77_OFFSET_DAY_OF_WEEK, 3);
	if(entry == 0x2a) {
		DEBUGPRINTF("bcdcorrect,ERROR11\n");
		return 0;
	}
	update_parity(&parity_date, entry);

	/* 45-48 -- month ones (0..9) -- nothing to check until tens */
	month_ones_entry = read_multiple(s, DCF77_OFFSET_MONTH_ONES, 4);
	update_parity(&parity_date, month_ones_entry);
	
	/* 49    -- month tens are all valid (0..1) -- nothing to check */
	entry = read_multiple(s, DCF77_OFFSET_MONTH_TENS, 1);
	/*
	 * If month tens is 1 then month ones is at most 2 or
	 * If month tens is 0 then month ones must not be 0
	 */
	if((entry == DCF77_BIT_1 && to_bcd(month_ones_entry) > 2) ||
			(entry == DCF77_BIT_0 && month_ones_entry == 0xaa)) {
		DEBUGPRINTF("bcdcorrect,ERROR11b,%x,%x,%d\n",
			entry, month_ones_entry, to_bcd(month_ones_entry));
		return 0;
	}
	update_parity(&parity_date, entry);

	/* 50-53 -- year ones are valid from 0..9 */
	entry = read_multiple(s, DCF77_OFFSET_YEAR_ONES, 4);
	if(to_bcd(entry) > 9) {
		DEBUGPRINTF("bcdcorrect,ERROR12,%d\n", to_bcd(entry));
		return 0;
	}
	update_parity(&parity_date, entry);

	/* 54-57 -- year tens are valid from 0..9 */
	entry = read_multiple(s, DCF77_OFFSET_YEAR_TENS, 4);
	if(to_bcd(entry) > 9) {
		DEBUGPRINTF("bcdcorrect,ERROR13,%d\n", to_bcd(entry));
		return 0;
	}
	update_parity(&parity_date, entry);

	update_parity(&parity_date, read_multiple(s,
						DCF77_OFFSET_PARITY_DATE, 1));

	/* date parity failed */
	if(parity_date == PARITY_SUM_ODD_MISM) {
		DEBUGPRINTF("bcdcorrect,ERROR14\n");
		return 0;
	}

	return 1;
}

static char check_end_of_minute(struct check_state* s)
{
	/*    59 -- End of minute (only regular ones accepted) */
	switch(read_multiple(s, DCF77_OFFSET_ENDMARKER_REGULAR, 1)) {
	case DCF77_BIT_0:
	case DCF77_BIT_1:
		DEBUGPRINTF("bcdcorrect,ERROR15,%d\n", read_multiple(s,
					DCF77_OFFSET_ENDMARKER_REGULAR, 1));
		return 0;
	default:
		return 1;
	}
}
