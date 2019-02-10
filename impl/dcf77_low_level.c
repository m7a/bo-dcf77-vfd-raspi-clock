#include <string.h>

/* TODO ASTAT ANEINANDERVORBEIGEHENDE KONZEPTE: UASWERTUNG AKTUELL ETWAS UNKLAR. EINFACHE ABER FUNKTIONSFÄHIGE VARIANTE GESUCHT... */

#include "interrupt.h"
#include "dcf77_low_level.h"

#define NUM_MEAS_MIN    120
#define NUM_DATA_MIN    100

#define NUM_MEAS_BIT    125
#define NUM_MEAS_MARKER 250

static int findstart(unsigned char start);
static unsigned char count_bits(unsigned char start, unsigned char end_excl,
							unsigned char bit);
static char is_within(unsigned char val, unsigned char upper,
							unsigned char lower);

void dcf77_low_level_init(struct dcf77_low_level* ctx)
{
	memset(ctx, 0, sizeof(struct dcf77_low_level));
}

enum dcf77_low_level_reading dcf77_low_level_proc(struct dcf77_low_level* ctx)
{
	enum dcf77_low_level_reading rv;
	unsigned char start;
	int locstart;
	char sb;
	char mb;

	if(interrupt_get_num_meas() < NUM_MEAS_MIN)
		return DCF77_LOW_LEVEL_NOTHING;

	rv = DCF77_LOW_LEVEL_NOTHING;
	start = interrupt_get_start();
	locstart = findstart(start);

	if(locstart != -1) {
		sb = is_within(locstart - start, NUM_MEAS_BIT,    4);
		mb = is_within(locstart - start, NUM_MEAS_MARKER, 5);
		if(sb || mb) {
			ctx->evalctr   = 0;
			ctx->mismatch  = 0;
			ctx->havestart = 1;
			rv = sb? proc_bit(locstart): proc_marker(locstart);
			interrupt_set_start(locstart);
		} else {
			ctx->mismatch++;
			if(ctx->mismatch >= 3 || !ctx->havestart) {
				ctx->evalctr   = 0;
				ctx->mismatch  = 0;
				ctx->havestart = 1;
				interrupt_set_start(locstart);
			}
		}
	}

	ctx->evalctr++;
	return rv;
}

static int findstart(unsigned char start)
{
	unsigned char next = interrupt_get_next();
	unsigned char i;
	unsigned char n;
	unsigned char n0;
	unsigned char n1;

	if(interrupt_get_num_between(start, next) < NUM_DATA_MIN)
		return -1;

	for(i = start; i != next; i++) {
		if(!interrupt_get_at(i) && interrupt_get_at(i + 1)) {
			n = i - start;
			n0 = count_bits(start, i, 0);
			if(n0 * 2 < n)
				continue;
			n = end - i;
			n1 = count_bits(i, end, 1)
			if(n1 * 2 < n)
				continue;

			return i;
		}
	}

	return -1;
}

static unsigned char count_bits(unsigned char start, unsigned char end_excl,
							unsigned char bit)
{
	unsigned char n = 0;
	unsigned char i;
	unsigned char val;
	for(i = start; i != end_excl; i++) {
		val = interrupt_get_at(i);
		if((val && bit) || (!val && !bit))
			n++;
	}
	return n;
}

static char is_within(unsigned char val, unsigned char upper,
							unsigned char lower)
{
	return val >= lower && val <= upper;
}

static enum dcf77_low_level_reading proc_bit(unsigned char locstart)
{
}

static enum dcf77_low_level_reading proc_marker(unsigned char locstart)
{
}

/* TODO proc_bit function */
