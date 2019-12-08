#include <string.h>

/*
#include "dcf77_proc_xeliminate.h"
#include "dcf77_telegram.h"
*/
#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_proc_moventries.h"

static void reset(struct dcf77_secondlayer* ctx);

void dcf77_secondlayer_init(struct dcf77_secondlayer* ctx)
{
	reset(ctx);
	ctx->fault_reset = 0; /* reset number of resets "the first is free" */
}

static void reset(struct dcf77_secondlayer* ctx)
{
	ctx->private_inmode               = IN_BACKWARD;
	ctx->private_line_current         = 0;
	ctx->private_line_cursor          = 59;
	ctx->private_leap_second_expected = 0; /* no leap second expected */
	/* denote number of resets */
	INC_SATURATED(ctx->fault_reset);
	/* initialize with 0 */
	memset(ctx->private_line_lengths,  0, DCF77_SECONDLAYER_LINES);
	/* initialize with epsilon */
	memset(ctx->private_telegram_data, 0, DCF77_SECONDLAYER_MEM);

	ctx->out_telegram_1_len = 0;
	ctx->out_telegram_2_len = 0;
}
