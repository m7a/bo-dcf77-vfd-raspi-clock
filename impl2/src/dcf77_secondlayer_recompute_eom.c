#include "dcf77_bitlayer.h"
#include "dcf77_telegram.h"
#include "dcf77_offsets.h"
#include "dcf77_secondlayer.h"
#include "dcf77_line.h"
#include "dcf77_proc_moventries.h"

/*
 * Precondition: Current line is "full", but does not end on eom.
 *
 * Needs to
 * (1) forward-identify a new eom marker starting from the end of the current
 *     first entry [which effectively means current+1 w/ skip empty]
 * (2) once identified, move all bits backwards the difference between the old
 *     EOM and the newly identified EOM. This way, some bits get shifted off at
 *     the first entry and thus the amount of data lessens. This should re-use
 *     existing work on leftward shifting.
 * (3) hand back to process_telegrams: this is not as trivial as it may sound.
 *     The problem is: there is actually no new telegram until the new "current"
 *     minute has finished. Thus will usually end there w/o returning new
 *     telegram data.
 *
 * This new implementation from 2020/11/01 does not handle leap seconds
 * gracefully to reduce complexity and enhance robustness...
 */
void dcf77_secondlayer_recompute_eom(struct dcf77_secondlayer* ctx)
{
	unsigned char telegram_start_offset_in_line;
	unsigned char start_line = dcf77_line_next(ctx->private_line_current);
	unsigned char line;
	unsigned char all_checked;

	if(ctx->private_leap_in_line != DCF77_SECONDLAYER_NOLEAP) {
		/*
		 * Some time ago, some telegram was processed as leap second
		 * telegram. No way to recover from this except by complex
		 * mid-data-interposition of leap sec bit. For the rarity of
		 * this case, ignore this and perform a reset.
		 */
		/* TODO RESET DISABLED FOR DEBUG ONLY */
		printf("reset(ctx)\n");
		exit(64);
	}

	all_checked = 0;
	for(telegram_start_offset_in_line = 1; (telegram_start_offset_in_line <
			60 && !all_checked); telegram_start_offset_in_line++) {
		all_checked = 1;
		for(line = start_line; (line != ctx->private_line_current) &&
				all_checked; line = dcf77_line_next(line)) {

			if(dcf77_line_is_empty(dcf77_line_pointer(ctx, line)))
				continue;

			if(!dcf77_secondlayer_check_bcd_correct_telegram(ctx,
					line, telegram_start_offset_in_line))
				all_checked = 0;
		}
	}

	/* slightly redundant check, just to be safe */
	if(all_checked && (telegram_start_offset_in_line != 60)) {
		/*
		 * Everyone needs to move telegram_start_offset_in_line steps to
		 * the left. This honors the length of lines and considers the
		 * case of a reduction of the* total number of lines.
		 */
		dcf77_secondlayer_move_entries_backwards(ctx,
						telegram_start_offset_in_line);
	} else {
		/*
		 * no suitable position found, data corruption or advanced
		 * leap sec case. requires reset
		 */
		/* TODO RESET DISABLED FOR DEBUG ONLY */
		printf("reset(ctx)\n");
		exit(64);
	}
}
