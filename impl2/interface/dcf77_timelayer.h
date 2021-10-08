/* requires dcf77_bitlayer.h */
/* requires dcf77_secondlayer.h */

#define DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN 10
#define DCF77_TIMELAYER_PREV_UNKNOWN -1

struct dcf77_timelayer_tm {
	short         y; /* Year    0..9999                    */
	unsigned char m; /* Month   1..12                      */
	unsigned char d; /* Day     1..31                      */
	unsigned char h; /* Hour    0..23                      */
	unsigned char i; /* Minute  0..59                      */
	short         s; /* Second  0..60 (60 = leapsec case)  */
};

enum dcf77_timelayer_qos {
	DCF77_TIMELAYER_QOS1 = 1, /* +1 -- perfectly synchronized */
	DCF77_TIMELAYER_QOS2 = 2, /* +2 -- synchronized minor disturbance */
	DCF77_TIMELAYER_QOS3 = 3, /* +3 -- synchronized from prev data */
	DCF77_TIMELAYER_QOS4 = 4, /* o4 -- recovered from prev */
	DCF77_TIMELAYER_QOS5 = 5, /* o5 -- async telegram match */
	DCF77_TIMELAYER_QOS6 = 6, /* o6 -- count from prev */
	DCF77_TIMELAYER_QOS7 = 7, /* -7 -- async count from last */
};

#ifndef DCF77_TIMELAYER_T_COMPILATION
#warning "DCF77_TIMELAYER_T_COMPILATION not set. Using 2021-09-11 00:24:43!"
#define DCF77_TIMELAYER_T_COMPILATION {.y=2021,.m=9,.d=11,.h=0,.i=24,.s=43}
#endif

struct dcf77_timelayer {
	/* ======================================================= private == */

	/*
	 * Ring buffer of last minute ones bits.
	 */
	unsigned char private_preceding_minute_ones[
					DCF77_TIMELAYER_LAST_MINUTE_BUF_LEN];
	unsigned char private_preceding_minute_idx;

	struct dcf77_timelayer_tm private_prev;
	unsigned char private_prev_telegram[DCF77_SECONDLAYER_LINE_BYTES];
	short private_num_seconds_since_prev;

	signed char seconds_left_in_minute; /* 0 = next minute */

	/* ======================================================== output == */

	struct dcf77_timelayer_tm out_current;
	enum dcf77_timelayer_qos qos;
};

void dcf77_timelayer_init(struct dcf77_timelayer* ctx);
void dcf77_timelayer_process(struct dcf77_timelayer* ctx,
					struct dcf77_bitlayer* bitlayer,
					struct dcf77_secondlayer* secondlayer);

#ifdef TEST
/* exported symbols for testing purposes */
char dcf77_timelayer_are_ones_compatible(unsigned char ones0,
							unsigned char ones1);
char dcf77_timelayer_is_leap_year(short y);
#endif
