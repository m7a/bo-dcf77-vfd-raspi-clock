/*
 * reads from 1 and 2 and writes to 2.
 * @return 0 if mismatch, 1 if OK
 */
char dcf77_secondlayer_xeliminate(unsigned char telegram_1_is_leap,
		unsigned char* in_telegram_1, unsigned char* in_out_telegram_2);

char dcf77_secondlayer_xeliminate_entry(unsigned char* in_1,
				unsigned char* in_out_2, unsigned char entry);
