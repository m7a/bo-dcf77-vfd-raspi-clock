#include "inc_sat.h"
#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_offsets.h"
#include "dcf77_line.h"
#include "dcf77_secondlayer_moventries.h"

void dcf77_secondlayer_move_entries_backwards(struct dcf77_secondlayer* ctx,
							unsigned char mov)
{
	unsigned char il0;     /* line in first inclusive */
	unsigned char il;      /* current input line */
	unsigned char pil;     /* position in input line */
	unsigned char readib;  /* read input byte position in memory */

	/* upper bits are moved "forward" and placed at a low position */
	unsigned char upper_low;
	/* lower bits will are moved "backward" and placed at a high position */
	unsigned char lower_up;

	unsigned char ol;
	unsigned char pol = 0; /* position in output line */

	unsigned char mov_entries       = (mov % 4);
	unsigned char mov_bytes_initial = (mov / 4);

	unsigned char shf;

	unsigned char bytes_proc = 0;
	unsigned char wrpos;
	
	/* -- Bestimme l0 -- */
	/*
	 * start from the first line in buffer.
	 * This is the first line following from the current which is not
	 * empty.
	 */
	for(il0 = dcf77_line_next(ctx->private_line_current);
		dcf77_line_is_empty(dcf77_line_pointer(ctx, il0)) &&
		il0 != ctx->private_line_current; il0 = dcf77_line_next(il0));

	ol = il0 * DCF77_SECONDLAYER_LINE_BYTES;

	/* -- Hauptprozedur -- */
	il = il0;
	do {
		/* skip empty input lines */
		if(dcf77_line_is_empty(dcf77_line_pointer(ctx, il)))
			continue;

		for(pil = 0; pil < DCF77_SECONDLAYER_LINE_BYTES; pil++) {
			/* -- Verwerfen der allerersten Eingaben */
			if(bytes_proc < mov_bytes_initial) {
				INC_SATURATED(bytes_proc);
				continue;
			}

			/* -- Verarbeiteprozedur: Lies Eingabe -- */
			readib = pil + il * DCF77_SECONDLAYER_LINE_BYTES;
			upper_low = (ctx->private_telegram_data[readib] >>
							(2 * mov_entries));
			shf = 8 - 2 * mov_entries;
			lower_up = (((0xff >> shf) &
				ctx->private_telegram_data[readib]) << shf);

			/* -- Verarbeiteprozedur: Schreibe Ausgabe -- */
			wrpos = pol + ol * DCF77_SECONDLAYER_LINE_BYTES;
			if(bytes_proc >= (mov_bytes_initial + 1))
				ctx->private_telegram_data[wrpos - 1] |=
								lower_up;
			/*
			 * Seit 24.10.2020 hier = statt |=, denn: Die Bytes
			 * müssen ja einmal im Zuge des Verschiebens auf 0
			 * gesetzt werden undd as scheint ein angemessener
			 * Zeitpunkt zu sein. Nacfolgend "nach hinten"
			 * verschobene Bytes (send_back_offset] |= lower_up)
			 * dürfen natürlich dann nicht auf 0 setzten, da sie
			 * sonst bereits vorhandene Daten überschreiben.
			 */
			ctx->private_telegram_data[wrpos] = upper_low;
			INC_SATURATED(bytes_proc);

			if(pol == (DCF77_SECONDLAYER_LINE_BYTES - 1)) {
				pol = 0;
				ol = dcf77_line_next(ol);
			} else {
				pol++;
			}
		}
	} while((il = dcf77_line_next(il)) != il0);
	
	/* -- Abschließende Aktualisierungen -- */
	ctx->private_line_cursor = pol;
	if(ctx->private_line_current != ol) {
		/* set previous line's length to 0 */
		dcf77_telegram_write_bit(DCF77_OFFSET_ENDMARKER_REGULAR,
			dcf77_line_pointer(ctx, ctx->private_line_current),
			DCF77_BIT_NO_UPDATE);
		ctx->private_line_current = ol;
	}
	
}
