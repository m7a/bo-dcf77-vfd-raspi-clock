/* requires dcf77_secondlayer.h */
/* requires dcf77_telegram.h */
/* requires dcf77_offsets.h */

static inline unsigned char dcf77_line_next(unsigned char inl)
{
	return (inl + 1) % DCF77_SECONDLAYER_LINES;
}

static inline char dcf77_line_is_empty(unsigned char* ptr_to_line)
{
	return dcf77_telegram_read_bit(0, ptr_to_line,
			DCF77_OFFSET_ENDMARKER_REGULAR) == DCF77_BIT_NO_UPDATE;
}

static unsigned char* dcf77_line_pointer(struct dcf77_secondlayer* ctx, unsigned char line)
{
	return ctx->private_line_data + (line * DCF77_SECONDLAYER_LINE_BYTES);
}
