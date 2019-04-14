#include <stdio.h>

#include "dcf77_low_level.h"

int main(int argc, char** argv)
{

}

/* interface */
#define DCF77_HIGH_LEVEL_MEM   138
#define DCF77_HIGH_LEVEL_LINES 9
#define DCF77_HIGH_LEVEL_TIME_LEN 8
#define DCF77_HIGH_LEVEL_DATE_LEN 10

enum dcf77_high_level_input_mode;

struct dcf77_high_level {
	/* private */
	enum dcf77_high_level_input_mode private_inmode;
	unsigned char private_telegram_data[DCF77_HIGH_LEVEL_MEM];
	unsigned char private_line_starts[DCF77_HIGH_LEVEL_LINES];
	unsigned char private_line_ends[DCF77_HIGH_LEVEL_LINES];
	unsigned char private_line_current;
	unsigned char private_line_cursor;
	/* input */
	enum dcf77_low_level_reading in_val;
	/* output */
	char out_time[DCF77_HIGH_LEVEL_TIME_LEN + 1]; /* TODO z requires copying to screen. Should avoid that by using just a pointer for one of them... Datetimelen is constant. */
	char out_date[DCF77_HIGH_LEVEL_DATE_LEN + 1];
};

void dcf77_high_level_init(struct dcf77_high_level* ctx);
void dcf77_high_level_process(struct dcf77_high_level* ctx);

/* internal */
enum dcf77_high_level_input_mode { IN_INIT, IN_ALIGNED, IN_UNKNOWN };

#define VAL_EPSILON 0 /* 00 */
#define VAL_X       1 /* 01 */
#define VAL_0       2 /* 10 */
#define VAL_1       3 /* 11 */

/* implementation */
void dcf77_high_level_init(struct dcf77_high_level* ctx)
{
	ctx->private_inmode = IN_INIT;
	ctx->private_line_starts[0] = 0;
	ctx->private_line_current = 0;
	ctx->private_line_cursor = 0;
	ctx->out_time[0] = 0;
	ctx->out_date[0] = 0;
}

void dcf77_high_level_process(struct dcf77_high_level* ctx)
{
	switch(ctx->private_inmode) {
	case IN_INIT:
		break;
	case IN_ALIGNED:
		break;
	case IN_UNKNOWN:
		break;
	}
}

/*
 * reads from 1 and 2 and writes to 2. Ignores semantics except for which bits
 * are considered.
 *
 * @return 0 if mismatch, 1 if OK
 */
static char xeliminate(size_t telegram_1_len, size_t telegram_2_len,
		unsigned char* in_telgram_1, unsigned char* in_out_telegram_2)
{
	if(!xeliminate_bits(in_telegram_1,     in_out_telegram_2, 1))
		return 0;

	if(!xeliminate_bits(in_telegram_1 + 4, in_out_telegram_2 + 4, 44))
		return 0;

	/* + endmarker / leap second TODO z*/
	if(telegram_1_len == telegram_2_len) {
	}
} 

static char xeliminate_bits(unsigned char* part1, unsigned char* part2, size_t num_bits)
{
/*
	TODO ASTAT
	size_t curbit = 0;
	size_t curbyteidx = 0;
	unsigned char curbyteval;

	while(curbit < num_bits) {
		curbyte = part1
	}
*/
}
