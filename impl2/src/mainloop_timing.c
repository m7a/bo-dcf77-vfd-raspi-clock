#include <stddef.h>
#include <stdint.h>

#include "mainloop_timing.h"

#define DELAY_MS_TARGET   100
#define DELAY_MS_VARIANCE  10

void mainloop_timing_init(struct mainloop_timing_ctx* ctx)
{
	ctx->delay_ms = DELAY_MS_TARGET;
	ctx->time_old = 0;
}

void mainloop_timing_pre(struct mainloop_timing_ctx* ctx, uint32_t time)
{
	ctx->time_new = time;
	ctx->delta_t  = ctx->time_new - ctx->time_old;
}

unsigned mainloop_timing_post_get_delay(struct mainloop_timing_ctx* ctx)
{
	ctx->time_old = ctx->time_new;

	if(!(DELAY_MS_TARGET - DELAY_MS_VARIANCE <= ctx->delta_t &&
			ctx->delta_t <= DELAY_MS_TARGET + DELAY_MS_VARIANCE))
		/* /3 -- do not change too rapidly */
		ctx->delay_ms += (DELAY_MS_TARGET - ctx->delta_t) / 3;

	/* Minimum 1ms delay for simulation integration */
	if(ctx->delay_ms <= 0)
		ctx->delay_ms = 1;

	return ctx->delay_ms;
}
