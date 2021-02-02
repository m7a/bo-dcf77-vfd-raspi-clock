#include "dcf77_offsets.h"
#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer_xeliminate.h"
#include "dcf77_telegram.h"
#include "debugprintf.h"

static char dcf77_secondlayer_xeliminate_begin_of_minute(
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2);
static char dcf77_secondlayer_xeliminate_match_from_to_except(
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2,
		unsigned char begin, unsigned char end, unsigned char except);
static char dcf77_secondlayer_xeliminate_daylight_saving_time(
					unsigned char* in_out_telegram_2);
static char dcf77_secondlayer_xeliminate_begin_time(
					unsigned char* in_out_telegram_2);
static char dcf77_secondlayer_xeliminate_end_of_minute(
		unsigned char telegram_1_is_leap, unsigned char* in_telegram_1,
		unsigned char* in_out_telegram_2);

/*
 * reads from 1 and 2 and writes to 2.
 * @return 0 if mismatch, 1 if OK
 */
char dcf77_secondlayer_xeliminate(unsigned char telegram_1_is_leap,
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2)
{
	return  dcf77_secondlayer_xeliminate_begin_of_minute(in_telegram_1,
							in_out_telegram_2) &&
		/* Skip begin leap second marker. Not used for xelimination */
		dcf77_secondlayer_xeliminate_match_from_to_except(
			in_telegram_1, in_out_telegram_2,
			16, 20, DCF77_OFFSET_LEAP_SEC_ANNOUNCE 
		) &&
		dcf77_secondlayer_xeliminate_daylight_saving_time(
							in_out_telegram_2) &&
		dcf77_secondlayer_xeliminate_begin_time(in_out_telegram_2) &&
		/* Skip parity minute, because minute ones may differ */
		dcf77_secondlayer_xeliminate_match_from_to_except(
			in_telegram_1, in_out_telegram_2,
			25, 58, DCF77_OFFSET_PARITY_MINUTE
		) &&
		dcf77_secondlayer_xeliminate_end_of_minute(telegram_1_is_leap,
					in_telegram_1, in_out_telegram_2);
} 

/* 0: entry has to match and be constant 0 */
static char dcf77_secondlayer_xeliminate_begin_of_minute(
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2)
{
	if(!dcf77_secondlayer_xeliminate_entry(in_telegram_1, in_out_telegram_2,
						DCF77_OFFSET_BEGIN_OF_MINUTE)) {
		DEBUGPRINTF("xeliminate,ERROR2,%02x,%02x\n", *in_telegram_1,
							*in_out_telegram_2);
		return 0;
	}

	switch(dcf77_telegram_read_bit(0, in_out_telegram_2)) {
	case DCF77_BIT_1:
		DEBUGPRINTF("xeliminate,ERROR3\n");
		return 0; /* constant 0 violated */
	case DCF77_BIT_NO_SIGNAL:
		/* correct to 0 */
		dcf77_telegram_write_bit(0, in_out_telegram_2, DCF77_BIT_0);
	}

	return 1;
}

char dcf77_secondlayer_xeliminate_entry(unsigned char* in_1,
				unsigned char* in_out_2, unsigned char entry)
{
	unsigned char val1 = dcf77_telegram_read_bit(entry, in_1);
	unsigned char val2 = dcf77_telegram_read_bit(entry, in_out_2);

	if(val1 == DCF77_BIT_NO_SIGNAL || val1 == DCF77_BIT_NO_UPDATE ||
								val1 == val2) {
		/* no update */
		return 1; /* OK */
	} else if(val2 == DCF77_BIT_NO_SIGNAL || val2 == DCF77_BIT_NO_UPDATE) {
		/* takes val 1 */
		dcf77_telegram_write_bit(entry, in_out_2, val1);
		return 1;
	}

	/* mismatch */
	return 0;
}

/* begin and end both incl */
static char dcf77_secondlayer_xeliminate_match_from_to_except(
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2,
		unsigned char begin, unsigned char end, unsigned char except)
{
	unsigned char i;
	for(i = begin; i <= end; i++) {
		if(i != except && !dcf77_secondlayer_xeliminate_entry(
				in_telegram_1, in_out_telegram_2, i)) {
			DEBUGPRINTF("xeliminate,ERROR4/%d,%d\n", except, i);
			return 0;
		}
	}
	return 1;
}

/* 17+18: needs to be 10 or 01 */
static char dcf77_secondlayer_xeliminate_daylight_saving_time(
					unsigned char* in_out_telegram_2)
{
	/*
	 * z Performance Optimization potential by reading both required bits
	 *   at once.
	 */
	unsigned char dst1 = dcf77_telegram_read_bit(
		DCF77_OFFSET_DAYLIGHT_SAVING_TIME, in_out_telegram_2);
	unsigned char dst2 = dcf77_telegram_read_bit(
		DCF77_OFFSET_DAYLIGHT_SAVING_TIME + 1, in_out_telegram_2);

	/* assertion violated if 00 or 11 found */
	if((dst1 == DCF77_BIT_0 && dst2 == DCF77_BIT_0) ||
				(dst1 == DCF77_BIT_1 && dst2 == DCF77_BIT_1)) {
		DEBUGPRINTF("xeliminate,ERROR5,d1=%u,d2=%u\n", dst1, dst2);
		return 0;
	}

	if(dst2 == DCF77_BIT_NO_SIGNAL && dst1 != DCF77_BIT_NO_SIGNAL
					&& dst1 != DCF77_BIT_NO_UPDATE) {
		/*
		 * use 17 to infer value of 18
		 * Write inverse value of dst1 (0->1, 1->0) to entry 2 in
		 * byte 4 (=18)
		 */
		dcf77_telegram_write_bit(DCF77_OFFSET_DAYLIGHT_SAVING_TIME + 1,
						in_out_telegram_2, dst1 ^ 1);
	} else if(dst1 == DCF77_BIT_NO_SIGNAL && dst2 != DCF77_BIT_NO_SIGNAL &&
						dst2 != DCF77_BIT_NO_UPDATE) {
		/* use 18 to infer value of 17 */
		dcf77_telegram_write_bit(DCF77_OFFSET_DAYLIGHT_SAVING_TIME,
						in_out_telegram_2, dst2 ^ 1);
	}

	return 1;
}

/* 20: entry has to match and be constant 1 */
static char dcf77_secondlayer_xeliminate_begin_time(
					unsigned char* in_out_telegram_2)
{
	switch(dcf77_telegram_read_bit(DCF77_OFFSET_BEGIN_TIME,
							in_out_telegram_2)) {
	case DCF77_BIT_0:
		DEBUGPRINTF("xeliminate,ERROR6\n");
		return 0; /* constant 1 violated */
	case DCF77_BIT_NO_SIGNAL:
		/* unset => correct to 1 */
		dcf77_telegram_write_bit(DCF77_OFFSET_BEGIN_TIME,
						in_out_telegram_2, DCF77_BIT_1);
	}
	return 1;
}

/* 59:entries have to match and be constant X (or special case leap second) */
static char dcf77_secondlayer_xeliminate_end_of_minute(
		unsigned char telegram_1_is_leap, unsigned char* in_telegram_1,
		unsigned char* in_out_telegram_2)
{
	unsigned char eom1;

	if(dcf77_telegram_read_bit(DCF77_OFFSET_ENDMARKER_REGULAR,
				in_out_telegram_2) != DCF77_BIT_NO_SIGNAL)
		return 0; /* telegram 2 cannot be leap, must end on no signal */

	eom1 = dcf77_telegram_read_bit(DCF77_OFFSET_ENDMARKER_REGULAR,
								in_telegram_1);
	return  (telegram_1_is_leap && (eom1 != DCF77_BIT_1)) ||
		(eom1 == DCF77_BIT_NO_SIGNAL);
}
