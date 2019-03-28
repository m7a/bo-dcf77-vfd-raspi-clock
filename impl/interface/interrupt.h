/* Pins to read DCF-77 signal from */
#define INTERRUPT_USE_PIN_DD   DDD2
#define INTERRUPT_USE_PIN_READ (PIND & _BV(PD2))

void interrupt_enable();
uint32_t interrupt_get_time_ms();
unsigned char interrupt_get_num_overflow();

/* for dcf77_low_level interface */
unsigned char interrupt_get_start();
void interrupt_set_start(unsigned char start);
unsigned char interrupt_get_num_meas();
unsigned char interrupt_get_num_between(unsigned char start,
							unsigned char next);
unsigned char interrupt_get_next();

/* returns 0 if 0 and a value different from 0 for 1 */
unsigned char interrupt_get_at(unsigned char idx);
