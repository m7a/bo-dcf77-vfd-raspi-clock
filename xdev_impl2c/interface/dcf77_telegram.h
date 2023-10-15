/* enum dcf77_bitlayer_reading, but works for other data as well */
static inline unsigned char dcf77_telegram_read_bit(
				unsigned char bit, unsigned char* telegram)
{
	const unsigned char entry = bit % 4;
	return (telegram[bit / 4] & (3 << (entry * 2))) >> (entry * 2);
}

/*
 * Auxiliary function to read individual numbers from telegrams without having
 * to align to char array boundaries.
 *
 * @param upper_low first byte to consider.
 * 	From this byte, the uppermost bits are used.
 *      "upper_low" means as much as
 * 	"the low byte from which we take the upper bits"
 * @param lower_up second byte to consider.
 * 	From this byte, the lowermost bits are used.
 *	"lower_up" means as much as
 * 	"the up byte from which we take the lower bits"
 * @param bit_offset Offset in telegram (0..59)
 * @param length Number of logical bits to read (1..4)
 */
static inline unsigned char dcf77_telegram_read_multiple_inner(
				unsigned char upper_low, unsigned char lower_up,
				unsigned char bit_offset, unsigned char length)
{
	unsigned char shf_ulow =     2 * (bit_offset % 4);
	unsigned char shf_loup = 8 - 2 * (bit_offset % 4);
	unsigned char shfl     = 8 - 2 * length;
	return (((upper_low & (0xff << shf_ulow)) >> shf_ulow) |
		((lower_up  & (0xff >> shf_loup)) << shf_loup)) &
		(0xff >> shfl);
}

static inline void dcf77_telegram_write_bit(unsigned char bit,
				unsigned char* telegram, unsigned char val)
{
	const unsigned char entry = bit % 4;
	telegram[bit / 4] = ( (telegram[bit / 4] & ~(3 << (entry * 2))) |
							(val << (entry * 2)) );
}
