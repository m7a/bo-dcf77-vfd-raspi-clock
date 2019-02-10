#include <stdint.h>
#include <string.h>
#include <stdio.h> /* TODO z only for sprintf temporarily imported */

/* TODO CSTAT AUSWERTEALGORITHMUS FUNKTIONIERT  NICHT, DA ES SICH NICHT RICHTIG SYNCHRONISIERT. EVENTUELL ERSTMAL EINEN EINFACHEREN ANSATZ MACHEN, DER EINFACH "ZUSAMMENHÄNGENDE" Folgen von Einsen erkennt und jeweils auswertet? Besser wäre es natürlich, den aktuellen Ansatz zu korrigieren, eventuell indem man bei findstart() einen Anfang mit 0en, eine Mitte mit 1en und ein Ende mit 0en erlaubt? -> könnte man ja einfach mal mit einer geschachtelten Suche testen, wenn zu lange dauert muss eben etwas einfacheres genommen werden? */

#include "interrupt.h"
#include "dcf77_low_level.h"

#define NUM_MEAS_BIT    125
#define NUM_MEAS_MARKER 250

static int findstart(unsigned char start);
static unsigned char count_bits(unsigned char start, unsigned char end_excl,
							unsigned char bit);
static char is_within(unsigned char val, unsigned char base, unsigned char pm);
static enum dcf77_low_level_reading proc_bit(struct dcf77_low_level* ctx,
				unsigned char start, unsigned char locstart);
static enum dcf77_low_level_reading proc_marker(struct dcf77_low_level* ctx,
				unsigned char start, unsigned char locstart);

static void debugchr(struct dcf77_low_level* ctx, char chr)
{
	ctx->debug[ctx->dbgidx % 5] = chr;
	ctx->debug[ctx->dbgidx = ((ctx->dbgidx + 1) % 5)] = '<';
}

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

	rv       = DCF77_LOW_LEVEL_NOTHING;
	start    = interrupt_get_start();
	locstart = findstart(start);

	if(locstart != -1) {
		sb = is_within(locstart - start, NUM_MEAS_BIT,    4);
		mb = is_within(locstart - start, NUM_MEAS_MARKER, 5);
		if(sb || mb) {
			ctx->evalctr   = 0;
			ctx->mismatch  = 0;
			if(ctx->havestart)
				rv = sb? proc_bit(ctx, start, locstart):
					proc_marker(ctx, start, locstart);
			ctx->havestart = 1;
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

	/*
	if( could be marker && ctx->evalctr >= 22) {
	}
	*/

	/*  else  */ if(ctx->evalctr >= 11) {
		/* procByteLost */
		debugchr(ctx, '_');
		ctx->evalctr = 0;
		/* TODO z might want to check that  start not set  beyond cursor pos? */
		interrupt_set_start(start + 120);
	}

	return rv;
}

static int findstart(unsigned char start)
{
	unsigned char next = interrupt_get_next();
	unsigned char i;
	unsigned char n;
	unsigned char n0;
	unsigned char n1;

	/* require at least 12 measurements, i.e. 12*8 = 96ms */
	if(interrupt_get_num_meas() < 12)
		return -1;

	/* TODO ASTAT THE PROBLEM MIGHT BE THIS: WE EXPECT OUR INPUT TO LOOK LIKE THIS ____---- but in reality it is ____--_____. This will never be detected as a valid ``start'' so what should we do? */

	for(i = start; i != next; i++) {
		if(!interrupt_get_at(i) && interrupt_get_at(i + 1)) {
			n = interrupt_get_num_between(start, i);
			n0 = count_bits(start, i, 0);
			if(n0 * 2 < n)
				continue;

			n = interrupt_get_num_between(i, next);
			n1 = count_bits(i, next, 1);
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

static char is_within(unsigned char val, unsigned char base, unsigned char pm)
{
	return ((base - pm) <= val) && (val <= (base + pm));
}

static enum dcf77_low_level_reading proc_bit(struct dcf77_low_level* ctx,
				unsigned char start, unsigned char locstart)
{
	unsigned char numBits = count_bits(start, locstart, 1);
	char buf[2];
	sprintf(buf, "%x", (numBits > 15? 0xf: numBits));
	debugchr(ctx, buf[0]);
	return DCF77_LOW_LEVEL_NOTHING; /* TODO z */
}

static enum dcf77_low_level_reading proc_marker(struct dcf77_low_level* ctx,
				unsigned char start, unsigned char locstart)
{
	/* not expected to occur during testing */
	debugchr(ctx, '!');
	return DCF77_LOW_LEVEL_NOTHING; /* TODO z */
}
