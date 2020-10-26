#include "inc_sat.h"
#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_nextl.h"
#include "dcf77_proc_moventries.h"

#define MOVENTRIES_ADVANCE_OUTPUT_CURSOR \
	{ \
		if(pol == 14) { \
			pol = 0; \
			if(next_produce_leapsec_marker) { \
				next_produce_leapsec_marker   = 0; \
				ctx->private_line_lengths[ol] = 61; \
			} else { \
				ctx->private_line_lengths[ol] = 60; \
			} \
			prevoll = ctx->private_line_lengths[ol]; \
			ol = dcf77_nextl(ol); \
		} else { \
			pol++; \
		} \
	}

/* TODO ASTAT TEST THIS PROCEDURE INDIVIDUALLY */
void dcf77_proc_move_entries_backwards(struct dcf77_secondlayer* ctx,
		unsigned char mov, unsigned char in_line_holding_leapsec_marker)
{
	unsigned char il0; /* line in first inclusive */

	/* previous input line length. init w/ 0 to silence compiler warning */
	unsigned char previll = 0;
	unsigned char il;      /* current input line */
	unsigned char ill;     /* current input line length */
	unsigned char pil;     /* position in input line */
	unsigned char bytes_to_proc;

	unsigned char readib;  /* read input byte position in memory */
	/*
	 * upper bytes will be moved "forward" and placed at a low position
	 * within the byte
	 */
	unsigned char upper_low;
	/*
	 * lower bytes will be moved "backward" and placed at a high position
	 * within the byte
	 */
	unsigned char lower_up;

	unsigned char next_produce_leapsec_marker; /* bool */
	unsigned char produce_leap; /* bool */
	unsigned char send_back_offset;
	unsigned char ol;
	unsigned char wrpos;

	unsigned char bytes_proc = 0;

	char prevoll = -1;     /* previous output line length */
	unsigned char pol = 0; /* position output line */

	unsigned char mov_entries = (mov % 4);
	unsigned char mov_bytes_initial = mov / 4;

	unsigned char shf;

	/* dumpmem(ctx); * TODO DEBUG ONLY */

	/* -- Bestimme l0 -- */

	/*
	 * start from the first line in buffer.
	 * This is the first line following from the current which is not
	 * empty.
	 */
	for(il0 = dcf77_nextl(ctx->private_line_current);
		ctx->private_line_lengths[il0] == 0 &&
		il0 != ctx->private_line_current; il0 = dcf77_nextl(il0));

	printf("    il0=%u, mov_entries=%u, mov_bytes_initial=%u\n", il0, mov_entries, mov_bytes_initial); /* TODO ASTAT DEBUG ONLY */

	ol = il0 * DCF77_SECONDLAYER_LINE_BYTES;

	/* -- Hauptprozedur -- */
	next_produce_leapsec_marker = 0;
	il = il0;
	do {
		next_produce_leapsec_marker |=
					(il == in_line_holding_leapsec_marker);
		ill = ctx->private_line_lengths[il];
		/* printf("    il=%u, ill=%u, next_produce_leapsec_marker=%u, ol=%u, pol=%u, bytes_proc=%u\n", il, ill, next_produce_leapsec_marker, ol, pol, bytes_proc); * DEBUG ONLY */
		/* ctx->private_line_lengths[il] = 0; * superflous? */

		/* skip empty input lines */
		if(ill == 0)
			continue;

		/* 
		 * 0..14 werden immer verarbeitet
		 * 15    nur bei leapsec und dann von einer separaten Logik
		 * ob man <60 oder <61 schreibt ist egal.
		 */
		bytes_to_proc = ill / 4 + (((ill % 4) != 0) && (ill < 60));
		/* printf("    bytes_to_proc = %d\n", bytes_to_proc); * 15 OK DEBUG */
		for(pil = 0; pil < bytes_to_proc; pil++) {
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
			if(pil == 0 && previll == 61) {	
				/* leap_in = X (might also want to read) */
				upper_low = ((upper_low << 2) |
							DCF77_BIT_NO_SIGNAL);
				mov--;
				mov_entries = mov % 4;
				if(mov_entries == 0)
					MOVENTRIES_ADVANCE_OUTPUT_CURSOR
			}
			/* -- Verarbeiteprozedur: Schreibe Ausgabe -- */
			produce_leap = ((prevoll == 61) && (pol == 0));
			send_back_offset = ((produce_leap || pol == 0)? 2: 1);
			wrpos = pol + ol * DCF77_SECONDLAYER_LINE_BYTES;
			if(bytes_proc >= (mov_bytes_initial + send_back_offset))
				ctx->private_telegram_data[wrpos -
						send_back_offset] |= lower_up;

			if(produce_leap) {
				if(bytes_proc >= (mov_bytes_initial + 1))
					ctx->private_telegram_data[wrpos - 1] =
							DCF77_BIT_NO_SIGNAL;
				upper_low <<= 2; /* cancel lowermost entry */
			}
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
			if(produce_leap) {
				mov++;
				mov_entries = (mov % 4);
				if(mov_entries == 0)
					continue; /* skip addr inc */
			}
			MOVENTRIES_ADVANCE_OUTPUT_CURSOR
		}
		previll = ill;

		/* dumpmem(ctx); * TODO DEBUG ONLY */
	} while((il = dcf77_nextl(il)) != il0);
	
	/* -- Abschließende Aktualisierungen -- */
	ctx->private_line_lengths[ol] = pol;
	ctx->private_line_cursor = pol;
	ctx->private_line_current = ol;

	/* dumpmem(ctx); * TODO DEBUG ONLY */
}
