#define DCF77_LOW_LEVEL_DIM_SERIES 140
#define DCF77_LOW_LEVEL_DEPTH_LOW  10
#define DCF77_LOW_LEVEL_DEPTH_HIGH 20

struct dcf77_low_level {
	unsigned char cursor;
	unsigned char series_high[DCF77_LOW_LEVEL_DIM_SERIES];
	unsigned char series_low[DCF77_LOW_LEVEL_DIM_SERIES];
	signed   char intervals_of_100ms_passed;
	unsigned char overflow;
};

enum dcf77_low_level_reading {

	/*
	 * corresponds to a "0" received from the receiver module
	 * ("1" for _1 respectively)
	 */
	DCF77_LOW_LEVEL_0 = 0,

	DCF77_LOW_LEVEL_1 = 1,

	/*
	 * _proc continued processing but a second has not passed yet
	 * (no update in the second counter necessary)
	 */
	DCF77_LOW_LEVEL_NO_UPDATE = 2,

	/*
	 * _proc detected that a second has passed but there was no signal
	 * detected from the receiver. This can mean two things:
	 * (1) some sort of disruption caused no signal to be decoded by the
	 *     receiver / receiver is not connected etc.
	 * (2) end of minute marker was received
	 */
	DCF77_LOW_LEVEL_NO_SIGNAL = 3,

};

void dcf77_low_level_init(struct dcf77_low_level* ctx);
enum dcf77_low_level_reading dcf77_low_level_proc(struct dcf77_low_level* ctx);
