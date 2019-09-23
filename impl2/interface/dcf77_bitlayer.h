/*
 * Transforms time values from interrupt into 0/1/X-values.
 * 
 * Input/Output is performed through variables in struct dcf77_bitlayer
 */

enum dcf77_bitlayer_reading {

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

struct dcf77_bitlayer {
	/* private */
	signed char private_intervals_of_100ms_passed;
	/* input: set before calling _proc */
	unsigned char in_val;
	unsigned char in_ticks_ago;
	/* output: read after calling _proc */
	enum dcf77_bitlayer_reading out_reading;
	/*
	 * If this is returned as 1, the access to interrupt data came in bad
	 * timing (too close to the interrupt writing the variables). Next
	 * time query later by e.g. adding some delay of 3 ticks or so (24ms)
	 */
	char out_misaligned;
	/*
	 * This is a status value/error counter for the number of times a
	 * signal's semantics could not be understood.
	 */
	unsigned char out_unidentified;
};

void dcf77_bitlayer_init(struct dcf77_bitlayer* ctx);
void dcf77_bitlayer_proc(struct dcf77_bitlayer* ctx);
