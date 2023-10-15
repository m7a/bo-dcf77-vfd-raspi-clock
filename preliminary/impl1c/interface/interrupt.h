/* Pins to read DCF-77 signal from */
#define INTERRUPT_USE_PIN_DD   DDD2
#define INTERRUPT_USE_PIN_READ (PIND & _BV(PD2))

void interrupt_enable();
uint32_t interrupt_get_time_ms();
unsigned char interrupt_get_num_overflow();

/*
 * new interface for DCF77 but should be invoked in main() rather than
 * dcf77_low_level.
 */
void interrupt_read_dcf77_signal(unsigned char* val, unsigned char* ticks_ago);
