/* Pins to read DCF-77 signal from */
#define INTERRUPT_USE_PIN_DD   DDD2
#define INTERRUPT_USE_PIN_READ (PIND & _BV(PD2))

void interrupt_enable();
uint32_t interrupt_get_time_ms();

/* for dcf77_low_level interface */
