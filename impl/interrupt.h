#define INTERRUPT_USE_PIN_DD DDD2

enum interrupt_dcf77_reading {
	INTERRUPT_DCF77_READING_0,
	INTERRUPT_DCF77_READING_1,
	INTERRUPT_DCF77_READING_MARKER,
	INTERRUPT_DCF77_READING_NOTHING
};

void interrupt_enable();
enum interrupt_dcf77_reading interrupt_read();
uint32_t interrupt_get_time_ms();
uint32_t interrupt_get_delta();
