#define DCF77_OFFSET_BEGIN_OF_MINUTE      0  /*     0 */
#define DCF77_OFFSET_DST_ANNOUNCE         16
#define DCF77_OFFSET_DAYLIGHT_SAVING_TIME 17 /* 17-18 */
#define DCF77_OFFSET_LEAP_SEC_ANNOUNCE    19
#define DCF77_OFFSET_BEGIN_TIME           20
#define DCF77_OFFSET_MINUTE_ONES          21
#define DCF77_OFFSET_MINUTE_TENS          25 /* 25-27 */
#define DCF77_OFFSET_PARITY_MINUTE        28
#define DCF77_OFFSET_HOUR_ONES            29
#define DCF77_OFFSET_HOUR_TENS            33
#define DCF77_OFFSET_PARITY_HOUR          35
#define DCF77_OFFSET_DAY_ONES             36
#define DCF77_OFFSET_DAY_TENS             40
#define DCF77_OFFSET_DAY_OF_WEEK          42
#define DCF77_OFFSET_MONTH_ONES           45
#define DCF77_OFFSET_MONTH_TENS           49
#define DCF77_OFFSET_YEAR_ONES            50
#define DCF77_OFFSET_YEAR_TENS            54
#define DCF77_OFFSET_PARITY_DATE          58
#define DCF77_OFFSET_ENDMARKER_REGULAR    59

#define DCF77_LENGTH_DAYLIGHT_SAVING_TIME 2
#define DCF77_LENGTH_MINUTE_ONES          4
#define DCF77_LENGTH_MINUTE_TENS          3
#define DCF77_LENGTH_HOUR_ONES            4
#define DCF77_LENGTH_HOUR_TENS            2
#define DCF77_LENGTH_DAY_ONES             4
#define DCF77_LENGTH_DAY_TENS             2
#define DCF77_LENGTH_DAY_OF_WEEK          3
#define DCF77_LENGTH_MONTH_ONES           4
#define DCF77_LENGTH_MONTH_TENS           1
#define DCF77_LENGTH_YEAR_ONES            4
#define DCF77_LENGTH_YEAR_TENS            4
