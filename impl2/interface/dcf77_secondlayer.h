#define DCF77_SECONDLAYER_LINES       9
#define DCF77_SECONDLAYER_TIME_LEN    8
#define DCF77_SECONDLAYER_DATE_LEN   10
#define DCF77_SECONDLAYER_LINE_BYTES 16
#define DCF77_SECONDLAYER_MEM        (DCF77_SECONDLAYER_LINE_BYTES * \
							DCF77_SECONDLAYER_LINES)

enum dcf77_secondlayer_input_mode {
	/* Init mode. Push data backwards */
	IN_BACKWARD,
	/* Aligned+Unknown mode. Push data forwards */
	IN_FORWARD
};

struct dcf77_secondlayer {
	/* private */
	enum dcf77_secondlayer_input_mode private_inmode;
	unsigned char private_telegram_data[DCF77_SECONDLAYER_MEM];
	/*
	 * the start and end are actually fixed because if we were to write
	 * continuously in the same manner, then some telegrams would start
	 * at offsets inside bytes. As we want to avoid this very much, there
	 * is instead the necessity to reogranize data in case a new marker
	 * is intended to be used as "end" marker. The lengths given here
	 * are in bits. The offsets of the lines are fixed which each line
	 * having 16 bytes available.
	 */
	unsigned char private_line_lengths[DCF77_SECONDLAYER_LINES];
	unsigned char private_line_current;
	unsigned char private_line_cursor;
	unsigned short private_leap_second_expected;

	/* input */
	enum dcf77_bitlayer_reading in_val;

	/*
	 * output
	 *
	 * Logic is a s follows:
	 * if only out_telegram_1_len is > 0, then process that as truth
	 * if both out_telegram_1_len and telegram_2_len are > 0 then
	 * 	process telegram_2 as truth and telegram_1 is from the
	 * 	previous 10min interval.
	 * if both are <0, then no new telegram data exists.
	 *
	 * implicit information
	 *
	 * has new second <=>
	 * 	has new measurement
	 * 	(lower layer information may bypass second layer here)
	 * has new telegram <=> out_telegram_1_len != 0
	 *
	 * For leap seconds, a regular telegram + length = 61 will be
	 * returned. This is as of now considered perfectly OK and stems
	 * from the xelimination not doing any elimination wrt. leapsec data.
	 *
	 * users should reset out_telegram_len values after processing!
	 */
	unsigned char out_telegram_1_len; /* in bits */
	unsigned char out_telegram_2_len; /* in bits */
	unsigned char out_telegram_1[DCF77_SECONDLAYER_LINE_BYTES];
	unsigned char out_telegram_2[DCF77_SECONDLAYER_LINE_BYTES];

	/* Number of resets performed (read-only output variable) */
	unsigned char fault_reset;
};

void dcf77_secondlayer_init(struct dcf77_secondlayer* ctx);
void dcf77_secondlayer_process(struct dcf77_secondlayer* ctx);

/* Exported symbols for testing purposes. Static for production. */
#ifdef TEST
void dcf77_secondlayer_reset(struct dcf77_secondlayer* ctx);
void dcf77_secondlayer_write_new_input(struct dcf77_secondlayer* ctx);
void dcf77_secondlayer_in_backward(struct dcf77_secondlayer* ctx);
#endif
