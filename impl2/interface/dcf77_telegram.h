/* enum dcf77_bitlayer_reading, but works for other data as well */
static inline unsigned char dcf77_telegram_read_bit(
				unsigned char bit, unsigned char* telegram)
{
	const unsigned char entry = bit % 4;
	return telegram[bit / 4] & (3 << (entry * 2)) >> (entry * 2);
}

static inline void dcf77_telegram_write_bit(unsigned char bit,
				unsigned char* telegram, unsigned char val)
{
	const unsigned char entry = bit % 4;
	telegram[bit / 4] = ((telegram[bit / 4] & ~(3 << entry)) |
								(val << entry));
}
