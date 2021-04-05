/* requires dcf77_secondlayer.h */
/* requires dcf77_telegram.h */
/* requires dcf77_offsets.h */

static inline unsigned char dcf77_line_next(unsigned char inl)
{
	return (inl + 1) % DCF77_SECONDLAYER_LINES;
}

static inline unsigned char dcf77_line_prev(unsigned char inl)
{
	return (inl == 0)? (DCF77_SECONDLAYER_LINES - 1): (inl - 1);
}

static inline char dcf77_line_is_empty(unsigned char* ptr_to_line)
{
	return dcf77_telegram_read_bit(0, ptr_to_line) == DCF77_BIT_NO_UPDATE;
}

static unsigned char* dcf77_line_pointer(struct dcf77_secondlayer* ctx,
							unsigned char line)
{
	return ctx->private_telegram_data +
					(line * DCF77_SECONDLAYER_LINE_BYTES);
}
