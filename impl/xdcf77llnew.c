#define DCF77_LOW_LEVEL_MATRIX_TIME_DIM 150
#define DCF77_LOW_LEVEL_MATRIX_DEPTH_LOW  5
#define DCF77_LOW_LEVEL_MATRIX_DEPTH_HIGH 30
#define DCF77_LOW_LEVEL_MATRIX_DEPTH_DIM \
	(DCF77_LOW_LEVEL_MATRIX_DEPTH_HIGH - DCF77_LOW_LEVEL_MATRIX_DEPTH_LOW)

struct dcf77_low_level {
	/*
	 * Counts the number of "1" bits seen depth cells into the future from
	 * a given time index.
	 *
	 * 1st index  v time
	 * 2nd index -> depth
	 */
	unsigned char matrix[DCF77_LOW_LEVEL_MATRIX_TIME_DIM]
				[DCF77_LOW_LEVEL_MATRIX_DEPTH_DIM];

	/* Whenever cursors overlap each other, the matrix is empty. */
	unsigned char matrix_read;  /* read cursor index */
	unsigned char matrix_write; /* write cursor index */

	signed char intervals_of_100ms_passed;
};

enum dcf77_low_level_reading {

	/*
	 * corresponds to a "0" received from the receiver module
	 * ("1" for _1 respectively)
	 */
	DCF77_LOW_LEVEL_0,

	DCF77_LOW_LEVEL_1,

	/*
	 * _proc continued processing but a second has not passed yet
	 * (no update in the second counter necessary)
	 */
	DCF77_LOW_LEVEL_NO_UPDATE,

	/*
	 * _proc detected that a second has passed but there was no signal
	 * detected from the receiver. This can mean two things:
	 * (1) some sort of disruption caused no signal to be decoded by the
	 *     receiver / receiver is not connected etc.
	 * (2) end of minute marker was received
	 */
	DCF77_LOW_LEVEL_NO_SIGNAL,

};

/* ----------------------------- IMPLEMENTATION ----------------------------- */

/*
 * Defines a matrix percentage limit at which we may accept an element as
 * "signal"
 */
#define DCF77_LOW_LEVEL_MATRIX_LIM_HEXPERC 191 /* ~ 75% */

void dcf77_low_level_init(struct dcf77_low_level* ctx)
{
	/* initialize by setting everything to 0 */
	memset(ctx, 0, sizeof(struct dcf77_low_level));
}

/*
 * REQUIRES
 * Invoke this every 100ms.
 *
 * PROVIDES
 * NO_UPDATE if no second has passed yet and 0/1/NO_SIGNAL about every second.
 * A jitter of +/- 100ms is to be expected in order to align the measurement
 * process with the signal timing.
 *
 * DEPENDS
 * Makes use of interrupt.h
 */
enum dcf77_low_level_reading dcf77_low_level_proc(struct dcf77_low_level* ctx)
{
	update_matrix(ctx);
	return process_second(ctx);
}

static void update_matrix(struct dcf77_low_level* ctx)
{
	unsigned char next = interrupt_get_next();
	unsigned char i;
	unsigned char j;
	for(i = interrupt_get_start(); i < next; i++) {
		/* Set all of the current elements to the measurement result */
		memset(ctx->matrix + ctx->matrix_write, interrupt_get_at(i),
					DCF77_LOW_LEVEL_MATRIX_DEPTH_DIM *
					sizeof(unsigned char));
		/* Counting backwards update past measurements */
		/* TODO CSTAT MATRIX UPDATE ROUTINE */
		/*
		for(j = 0; j < DCF77_LOW_LEVEL_MATRIX_DEPTH_DIM; j++) {
			
		}
		*/
	}
}

static unsigned char time_idx_forward(unsigned char idx)
{
	return (idx + 1) % DCF77_LOW_LEVEL_MATRIX_TIME_DIM;
}

/*
static unsigned char time_idx_backward(unsigned char idx)
{
	return (idx > 0)? (idx - 1): (DCF77_LOW_LEVEL_MATRIX_TIME_DIM - 1);
}
*/

static enum dcf77_low_level_reading process_second(struct dcf77_low_level* ctx)
{
	enum dcf77_low_level_reading rv;
	
	unsigned char findpos;

	if(++ctx->intervals_of_100ms_passed < 10)
		return DCF77_LOW_LEVEL_NO_UPDATE;

	if(matrix_num_elements(ctx) < DCF77_LOW_LEVEL_MATRIX_DEPTH_HIGH) {
		/*
		 * Too few elements in matrix and second has already passed.
		 * Next time please query later.
		 */
		next_time_query_later(ctx);
		return DCF77_LOW_LEVEL_NO_SIGNAL;
	} else {
		/* Enough elements to check the matrix for elements */

		/* Decode bit */
		findpos = findn(ctx, DCF77_LOW_LEVEL_MATRIX_DEPTH_HIGH);
		if(findpos == DCF77_LOW_LEVEL_MATRIX_TIME_DIM) {
			/* no high count found (no 1 bit detected) */
			findpos = findn(ctx, DCF77_LOW_LEVEL_MATRIX_DEPTH_LOW);
			if(findpos == DCF77_LOW_LEVEL_MATRIX_TIME_DIM) {
				/* no 1 and no 0 detected */
				rv = DCF77_LOW_LEVEL_NO_SIGNAL;
			} else {
				/* low count found (0 bit detected) */
				rv = DCF77_LOW_LEVEL_0;
			}
		} else {
			/* high count found (1 bit detected) */
			rv = DCF77_LOW_LEVEL_1;
		}

		/* Update timing */
		/*
		 * TODO z beware that this comes into effect after the next
		 * measurement has finished at the earliest!
		 */
		if(findpos <= 3)
			next_time_query_earlier();
		else if(findpos > DCF77_LOW_LEVEL_MATRIX_TIME_DIM/2)
			next_time_query_later();
		else
			ctx->intervals_of_100ms_passed = 0; /* query same */

		/* Discard buffer */
		ctx->matrix_read = ctx->matrix_write;

		return rv;
	}
}

static unsigned char matrix_num_elements(struct dcf77_low_level* ctx)
{
	return ctx->matrix_write - ctx->matrix_read;
}

static void next_time_query_earlier(struct dcf77_low_level* ctx)
{
	ctx->intervals_of_100ms_passed = 1;
}

static void next_time_query_later(struct dcf77_low_level* ctx)
{
	ctx->intervals_of_100ms_passed = -1;
}

/*
 * Check if there is any location where n 1es have been detected
 * (or at least to HEXPERC hex-percent)
 *
 * @return DCF77_LOW_LEVEL_MATRIX_TIME_DIM if none found.
 */
static unsigned char findn(struct dcf77_low_level* ctx, unsigned char n)
{
	unsigned char t;
	unsigned short matrix_perc;

	/* t_max out of range for time series [0;_DIM-1] => invalid */
	unsigned char t_max = DCF77_LOW_LEVEL_MATRIX_TIME_DIM;
	/* need to find at least HEXPERC */
	unsigned short matrix_perc_max = DCF77_LOW_LEVEL_MATRIX_LIM_HEXPERC;

	for(t = ctx->matrix_read; t != ctx->matrix_write;
						t = time_idx_forward()) {
		matrix_perc = ctx->matrix[t][n -
				DCF77_LOW_LEVEL_MATRIX_DEPTH_LOW] * 0xff / n;
		if(matrix_perc > matrix_perc_max) {
			matrix_perc_max = matrix_perc;
			t_max = t;
		}
	}

	return t_max;
}
