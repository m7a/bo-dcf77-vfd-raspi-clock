/*
 * Takes up to four "bit" of internally represented data and transforms them to
 * unsinged C byte where 0 is BCD-0, 1 is BCD-1 etc.
 *
 * "&" adjacent bits returns 1 exactly if it is DCF77_BIT_1.
 * move the obtained ones to the lower output parts s.t. output values range
 * from 0-15.
 */
static inline unsigned char dcf77_bcd_from(unsigned char by)
{
	return  (((by & 0x01) & (by & 0x02) >> 1) >> 0) |
		(((by & 0x04) & (by & 0x08) >> 1) >> 1) |
		(((by & 0x10) & (by & 0x20) >> 1) >> 2) |
		(((by & 0x40) & (by & 0x80) >> 1) >> 3);
}
