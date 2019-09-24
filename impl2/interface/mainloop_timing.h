struct mainloop_timing_ctx {
	/* private */
	int      delay_ms;
	uint32_t time_old;
	uint32_t time_new;
	int      delta_t;
};

void mainloop_timing_init(struct mainloop_timing_ctx* ctx);
void mainloop_timing_pre(struct mainloop_timing_ctx* ctx, uint32_t time);
unsigned mainloop_timing_post_get_delay(struct mainloop_timing_ctx* ctx);
