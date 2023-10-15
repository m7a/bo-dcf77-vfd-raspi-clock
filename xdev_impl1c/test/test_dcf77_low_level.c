/* This test is currently defunct */
#include <stdio.h>
#include <stdint.h>

#include "interrupt.h"
#include "dcf77_low_level.h"

#define _BV(N) (1 << (N))

/* 125 measurements/second */
/* hight: ~30x1, low: ~5x1 */
#define MEAS_PER_SEC 125
#define TESTSEQLEN 6
static char testseq[TESTSEQLEN][MEAS_PER_SEC] = {
	/* a 0 */
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	/* a 1 */
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
	/* a 0 */
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	/* a 1 */
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
	/* an endmarker (2 sec?) */
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

/* very much like the real interrupt */
#define SIZE_BYTES 32
static unsigned char interrupt_readings[SIZE_BYTES];
static unsigned char interrupt_start        = 0;
static unsigned char interrupt_next         = 0;
static unsigned char interrupt_num_overflow = 0;

static void add_bit_from_interrupt(char bitval);

int main(int argc, char** argv)
{
	int twelves = 0;
	int i;
	int j;
	enum dcf77_low_level_reading rd;
	struct dcf77_low_level ctx;
	dcf77_low_level_init(&ctx);

	for(i = 0; i < TESTSEQLEN; i++) {
		for(j = 0; j < MEAS_PER_SEC; j++) {
			add_bit_from_interrupt(testseq[i][j]);
			if(++twelves >= 12) {
				printf("[%2d|%3d] INVOKE READING ", i, j);
				fflush(stdout);
				rd = dcf77_low_level_proc(&ctx);
				switch(rd) {
				case DCF77_LOW_LEVEL_0:
					puts("-> READING      0"); break;
				case DCF77_LOW_LEVEL_1:
					puts("-> READING      1"); break;
				case DCF77_LOW_LEVEL_NO_UPDATE:
					puts("-> NO_UPDATE");      break;
				case DCF77_LOW_LEVEL_NO_SIGNAL:
					puts("-> NO_SIGNAL    _"); break;
				default:
					puts("-> ?");
				}
				twelves = 0;
			}
		}
	}
	
	return 0;
}

static void add_bit_from_interrupt(char bitval)
{
	unsigned char idxh = interrupt_next >> 3;
	unsigned char idxl = interrupt_next & 7;

	interrupt_readings[idxh] = (interrupt_readings[idxh] & ~_BV(idxl)) |
					(bitval << idxl);

	if(++interrupt_next == interrupt_start) {
		interrupt_num_overflow = ((interrupt_num_overflow == 0xff)?
					0xff: (interrupt_num_overflow + 1));
		printf("-- OVERFLOW=%d --", interrupt_num_overflow);
	}
}

unsigned char interrupt_get_start()
{
	return interrupt_start;
}

unsigned char interrupt_get_next()
{
	return interrupt_next;
}

unsigned char interrupt_get_at(unsigned char idx)
{
	return interrupt_readings[idx >> 3] & (_BV(idx & 7));
}

void interrupt_set_start(unsigned char start)
{
	interrupt_start = start;
}
