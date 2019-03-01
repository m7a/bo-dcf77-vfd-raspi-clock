/* Defines a percentage limit at which we may accept an element as "signal" */
#define DCF77_LOW_LEVEL_LIM_HEXPERC 191 /* ~ 75% */

static void process_measured_value(unsigned char val, unsigned char cursor,
					unsigned char* series, unsigned char n);
static enum dcf77_low_level_reading process_second(struct dcf77_low_level* ctx);
static void next_time_query_earlier(struct dcf77_low_level* ctx);
static void next_time_query_later(struct dcf77_low_level* ctx);
static unsigned char findn(unsigned char cursor, unsigned char* series, 
							unsigned char n);

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
	unsigned char iidx;
	unsigned char val;
	for(iidx = interrupt_get_start(); iidx != interrupt_get_next();
								iidx++) {
		if(ctx->cursor == DCF77_LOW_LEVEL_DIM_SERIES) {
			if(ctx->overflow < 0xff)
				ctx->overflow++;
			break;
		}
		val = interrupt_get_at(iidx);
		process_measured_value(val, ctx->cursor, ctx->series_low,
						DCF77_LOW_LEVEL_DEPTH_LOW);
		process_measured_value(val, ctx->cursor, ctx->series_high,
						DCF77_LOW_LEVEL_DEPTH_HIGH);
		ctx->cursor++;
	}
	return process_second(ctx);
}

static void process_measured_value(unsigned char val, unsigned char cursor,
					unsigned char* series, unsigned char n)
{
	unsigned char i;
	series[cursor] = 0;
	for(i = cursor; i >= 0 && (cursor - i) < n; i--)
		series[i] += val;
}

static enum dcf77_low_level_reading process_second(struct dcf77_low_level* ctx)
{
	enum dcf77_low_level_reading rv;
	
	unsigned char findpos;

	if(++ctx->intervals_of_100ms_passed < 10)
		return DCF77_LOW_LEVEL_NO_UPDATE;

	if(ctx->cursor < DCF77_LOW_LEVEL_DEPTH_HIGH) {
		/*
		 * Too few elements in series and second has already passed.
		 * Next time please query later.
		 */
		next_time_query_later(ctx);
		ctx->cursor = 0; /* discard buffer */
		return DCF77_LOW_LEVEL_NO_SIGNAL;
	} else {
		/* Enough elements to check the series for elements */

		/* Decode bit */
		findpos = findn(ctx->cursor, ctx->series_high,
						DCF77_LOW_LEVEL_DEPTH_HIGH);
		if(findpos == DCF77_LOW_LEVEL_DIM_SERIES) {
			/* no high count found (no 1 bit detected) */
			findpos = findn(ctx->cursor, ctx->series_low,
						DCF77_LOW_LEVEL_DEPTH_LOW);
			if(findpos == DCF77_LOW_LEVEL_DIM_SERIES) {
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
		else if(findpos > (DCF77_LOW_LEVEL_DIM_SERIES / 2))
			next_time_query_later();
		else
			ctx->intervals_of_100ms_passed = 0; /* query same */

		ctx->cursor = 0; /* discard buffer */
		return rv;
	}
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
 * @return DCF77_LOW_LEVEL_DIM_SERIES if none found.
 */
static unsigned char findn(unsigned char cursor, unsigned char* series, 
								unsigned char n)
{
	unsigned char i;
	unsigned short perc;

	/* t_max out of range for time series [0;_DIM-1] => invalid */
	unsigned char t_max = DCF77_LOW_LEVEL_DIM_SERIES;
	/* need to find at least HEXPERC */
	unsigned short perc_max = DCF77_LOW_LEVEL_LIM_HEXPERC;

	for(i = n; i < cursor; i++) {
		perc = series[i] * 0xff / n;
		if(perc > perc_max) {
			perc_max = perc;
			/*
			 * As the series are backwards-populated the starting
			 * point in question is actually i - n
			 */
			t_max = i - n;
		}
	}

	return t_max;
}
