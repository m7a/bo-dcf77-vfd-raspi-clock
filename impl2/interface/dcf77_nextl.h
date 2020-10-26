/* requires dcf77_secondlayer.h */
static inline unsigned char dcf77_nextl(unsigned char inl)
{
	return (inl + 1) % DCF77_SECONDLAYER_LINES;
}
