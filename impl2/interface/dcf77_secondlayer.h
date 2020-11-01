#define DCF77_SECONDLAYER_LINES       9
#define DCF77_SECONDLAYER_NOLEAP      9
#define DCF77_SECONDLAYER_LINE_BYTES 15
#define DCF77_SECONDLAYER_MEM        (DCF77_SECONDLAYER_LINE_BYTES * \
							DCF77_SECONDLAYER_LINES)

enum dcf77_secondlayer_input_mode {
	IN_BACKWARD,   /* Init mode. Push data backwards */
	IN_FORWARD     /* Aligned+Unknown mode. Push data forwards */
};

struct dcf77_secondlayer {
	/* ======================================================= private == */
	enum dcf77_secondlayer_input_mode private_inmode;
	unsigned char private_telegram_data[DCF77_SECONDLAYER_MEM];

	/*
	 * Current line to work on
	 */
	unsigned char private_line_current;

	/*
	 * Position in line given in data points “bits”.
	 * Ranges from 0 to 59 both incl.
	 *
	 * In forward mode, this sets the position to write the next bit to.
	 *
	 * In backward mode, this sets the position to move the earliest bit to
	 * after writing the new bit to the end of the first line.
	 */
	unsigned char private_line_cursor;

	/*
	 * Gives the index of a line that has a leap second.	
	 *
	 * The additional NO_SIGNAL for the leap second is not stored
	 * anywhere, but only reflected by this value. As long as there is no
	 * leap second recorded anywhere, this field has value
	 * DCF77_SECONDLAYER_NOLEAP (9).
	 */
	unsigned char private_leap_in_line;

	/*
	 * Leap second announce timer/countdown in seconds.
	 *
	 * Set to 70 * 60 seconds upon first seeing a leap second announce bit.
	 * This way, it will expire ten minutes after the latest point in time
	 * where the leap second could occur.
	 */
	unsigned short private_leap_second_expected;

	/* ========================================================= input == */
	enum dcf77_bitlayer_reading in_val;

	/*
	 * ======================================================== output == *
	 *
	 * Logic is a s follows:
	 * out_telegram_1 is always the truth if len != 0
	 * out_telegram_2 contains data from previous minute if len != 0
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
