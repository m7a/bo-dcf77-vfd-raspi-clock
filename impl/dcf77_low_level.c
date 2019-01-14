#include <string.h>

#include "interrupt.h"
#include "dcf77_low_level.h"

void dcf77_low_level_init(struct dcf77_low_level* ctx)
{
	memset(ctx, 0, sizeof(struct dcf77_low_level));
}

enum dcf77_low_level_reading dcf77_low_level_proc(struct dcf77_low_level* ctx)
{
	enum dcf77_low_level_reading rv;
	unsigned char start = interrupt_get_start();
	int locstart = findstart(start);
	char sb;
	char mb;

	if(locstart != -1) {
		sb = is_within(locstart - start, 125, 4);
		mb = is_within(locstart - start, 250, 5);
		if(sb || mb) {
			ctx->evalctr = 0;
			ctx->mismatch = 0;
			ctx->havestart = 1;
			rv = sb? proc_byte(start, locstart):
						proc_marker(start, locstart);
			interrupt_set_start(locstart);
		} else {
			ctx->mismatch++;
			if(ctx->mismatch >= 3 || !ctx->havestart) {
				ctx->evalctr = 0;
				ctx->mismatch = 0;
				ctx->havestart = 1;
				interrupt_set_start(locstart);
			}
		}
	}

	ctx->evalctr++;
	/* TODO ... */
}

static int findstart(unsgined char start)
{
	unsigned char next = interrupt_get_next();
	unsigned char i;
	unsigned char n;
	unsigned char n0;
	unsigned char n1;

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
