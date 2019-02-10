enum dcf77_low_level_reading {
	DCF77_LOW_LEVEL_0,
	DCF77_LOW_LEVEL_1,
	DCF77_LOW_LEVEL_MARKER,
	DCF77_LOW_LEVEL_UNKNOWN,
	DCF77_LOW_LEVEL_UNKNOWN_2,
	DCF77_LOW_LEVEL_NOTHING
};

struct dcf77_low_level {
	unsigned char havestart;
	unsigned char evalctr;
	unsigned char mismatch;

	unsigned char dbgidx;
	char debug[6];
};

void dcf77_low_level_init(struct dcf77_low_level* ctx);

/* Should be invoked around every 100ms */
enum dcf77_low_level_reading dcf77_low_level_proc(struct dcf77_low_level* ctx);
