#include "dcf77_bitlayer.h"
#include "dcf77_proc_xeliminate.h"

static char xeliminate_entry(unsigned char in_1, unsigned char* in_out_2,
							unsigned char entry);
static unsigned char read_entry(unsigned char in, unsigned char entry);

/*
 * reads from 1 and 2 and writes to 2.
 *
 * @param telegram_len number of entries in telegram (60 or 61)
 * @return 0 if mismatch, 1 if OK
 */
char dcf77_proc_xeliminate(
		unsigned char telegram_1_len, unsigned char telegram_2_len,
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2)
{
	unsigned char i;
	unsigned char etmp;
	unsigned char etmp2;
	unsigned char* telleap;
	unsigned char* telregular;

	/* 0:    entry has to match and be constant 0 */
	if(!xeliminate_entry(*in_telegram_1, in_out_telegram_2, 0)) {
		/* printf("<<<ERROR2,%02x,%02x>>>\n", *in_telegram_1,
							*in_out_telegram_2); */
		return 0;
	}

	etmp = read_entry(*in_out_telegram_2, 0);
	if(etmp == DCF77_BIT_1) {
		/* puts("<<<ERROR3>>>"); */
		return 0; /* constant 0 violated */
	} else if(etmp == DCF77_BIT_NO_SIGNAL) {
		/* correct to 0 */
		*in_out_telegram_2 = (*in_out_telegram_2 & ~3) | DCF77_BIT_0;
	}

	/* 16--20: entries have to match */
	for(i = 16; i <= 20; i++) {
		/*
		 * i != 19: skip leap second marker. for now it is not used for
		 *          correction...
		 */
		if(i != 19 && !xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4)) {
			/* printf("<<<ERROR4,%d>>>\n", i); */
			return 0;
		}
	}

	/* 17+18: needs to be 10 or 01 */
	etmp  = read_entry(in_out_telegram_2[4], 1);
	etmp2 = read_entry(in_out_telegram_2[4], 2);
	/* assertion violated if 00 or 11 found */
	if((etmp == DCF77_BIT_0 && etmp2 == DCF77_BIT_0) ||
				(etmp == DCF77_BIT_1 && etmp2 == DCF77_BIT_1)) {
		/* printf("<<<ERROR5,etmp1=%u,etmp2=%u>>>\n", etmp, etmp2); */
		return 0;
	}
	if(etmp2 == DCF77_BIT_NO_SIGNAL && etmp != DCF77_BIT_NO_SIGNAL
					&& etmp != DCF77_BIT_NO_UPDATE) {
		/*
		 * use 17 to infer value of 18
		 *
		 * Write inverse value of etmp (0->1, 1->0) to entry 2 in
		 * byte 4 (=18)
		 */
		in_out_telegram_2[4] =
			(in_out_telegram_2[4] & 0xcf) | ((etmp ^ 1) << 4);
	} else if(etmp == DCF77_BIT_NO_SIGNAL && etmp2 != DCF77_BIT_NO_SIGNAL &&
						etmp2 != DCF77_BIT_NO_UPDATE) {
		/* use 18 to infer value of 17 */
		in_out_telegram_2[4] = (in_out_telegram_2[4] & 0xf3) |
							((etmp2 ^ 1) << 2);
	}

	/* 20:     entry has to match and be constant 1 */
	etmp = read_entry(in_out_telegram_2[5], 0);
	if(etmp == DCF77_BIT_0) {
		/* puts("<<<ERROR6>>>"); */
		return 0; /* constant 1 violated */
	} else if(etmp == DCF77_BIT_NO_SIGNAL) {
		/* unset => correct to 1 */
		in_out_telegram_2[5] = (in_out_telegram_2[5] & ~3) |
								DCF77_BIT_1;
	}

	/* 25--58: entries have to match */
	for(i = 25; i <= 58; i++) {
		/*
		 * i != 28: except for minute parity bit (28) which may change
		 *          depending on minute unit value
		 */
		if(i != 28 && !xeliminate_entry(in_telegram_1[i / 4],
					in_out_telegram_2 + (i / 4), i % 4)) {
			/* printf("<<<ERROR7,i=%u>>>\n", i); */
			return 0;
		}
	}

	/* 59:   entries have to match and be constant X
	 *       (or special case leap second) */
	if(telegram_1_len == 60 && telegram_2_len == 60) {
		/* needs to be X -- regular mintues */
		return (read_entry(in_telegram_1[14], 3) ==
			DCF77_BIT_NO_SIGNAL && read_entry(
			in_out_telegram_2[14], 3) == DCF77_BIT_NO_SIGNAL);
	} else if((telegram_1_len == 61 && telegram_2_len == 60) ||
			(telegram_1_len == 60 && telegram_2_len == 61)) {
		/* Check the larger one */
		if(telegram_1_len == 61) {
			telleap    = in_telegram_1;
			telregular = in_out_telegram_2;
		} else {
			telleap    = in_out_telegram_2;
			telregular = in_telegram_1;
		}
		/*
		 * Now the check to perform is that the added part
		 * just before the end marker needs to be a `0` (X is
		 * allowed as well)
		 *
		 * Leap seconds are rare. We do not use them to
		 * correct data in the telegram (does not make sense
		 * to write to telegram_1 which is an in-variable
		 * anyways).
		 */
		return (read_entry(telleap[14],    3) == DCF77_BIT_NO_SIGNAL ||
			read_entry(telleap[14],    3) == DCF77_BIT_0)        &&
								/* marker */
			read_entry(telleap[15],    0) == DCF77_BIT_NO_SIGNAL &&
			read_entry(telregular[14], 3) == DCF77_BIT_NO_SIGNAL;
	} else {
		/* printf("<<<ERROR8>>>\n"); */
		/* not a minute's telegram -> invalid */
		return 0;
	}
} 

static char xeliminate_entry(unsigned char in_1, unsigned char* in_out_2,
							unsigned char entry)
{
	unsigned char val1 = read_entry(in_1,      entry);
	unsigned char val2 = read_entry(*in_out_2, entry);

	if(val1 == DCF77_BIT_NO_SIGNAL || val1 == DCF77_BIT_NO_UPDATE ||
								val1 == val2) {
		/* no update */
		return 1; /* OK */
	} else if(val2 == DCF77_BIT_NO_SIGNAL || val2 == DCF77_BIT_NO_UPDATE) {
		/* takes val 1 */
		*in_out_2 = (*in_out_2 & ~(3 << (entry * 2))) |
							(val1 << (entry * 2));
		return 1;
	}

	/* mismatch */
	return 0;
}

static unsigned char read_entry(unsigned char in, unsigned char entry)
{
	return (in & (3 << (entry * 2))) >> (entry * 2);
}
