/* internal */
#define DCF77_SECONDLAYER_VAL_EPSILON 0 /* 00 */
#define DCF77_SECONDLAYER_VAL_X       1 /* 01 */
#define DCF77_SECONDLAYER_VAL_0       2 /* 10 */
#define DCF77_SECONDLAYER_VAL_1       3 /* 11 */

char dcf77_proc_xeliminate(
		unsigned char telegram_1_len, unsigned char telegram_2_len,
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2);
