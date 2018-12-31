/* I/O configuration */
#define INPUT_MUX_SENSOR         (_BV(MUX2) |             _BV(MUX0)) /* ADC5 */
#define INPUT_MUX_MODE_SELECTOR  (_BV(MUX2) | _BV(MUX1)            ) /* ADC6 */
#define INPUT_MUX_BUTTONS        (_BV(MUX2) | _BV(MUX1) | _BV(MUX0)) /* ADC7 */

enum input_mode {
	INPUT_MODE_INVALID                = 0,
	INPUT_MODE_NORMAL                 = 1,
	INPUT_MODE_ALARM                  = 2,
	INPUT_MODE_ALARM_SET_TIME         = 3,
	INPUT_MODE_STATUS_SET_HOUR_MINUTE = 4,
	INPUT_MODE_STATUS_SET_DAY_MONTH   = 5,
	INPUT_MODE_STATUS_SET_YEAR        = 6,
};

enum input_button_press {
	INPUT_BUTTON_INVALID = 0,
	INPUT_BUTTON_NONE    = 1,
	INPUT_BUTTON_1_ONLY  = 2,
	INPUT_BUTTON_2_ONLY  = 3,
	INPUT_BUTTON_BOTH    = 4,
};

struct input {
	/*
	 * makes available the raw measurements.
	 * It is recommended to only read from this structure.
	 */
	unsigned char mode;
	unsigned char btn;
	unsigned char sensor;
};

void input_init(struct input* ctx);

/* @return INVALID if it is just being turned */
enum input_mode input_read_mode(struct input* ctx);

/* @return INVALID if out of range (see raw value then) */
enum input_button_press input_read_buttons(struct input* ctx);

/* @return raw measurement */
unsigned char input_read_sensor(struct input* ctx);
