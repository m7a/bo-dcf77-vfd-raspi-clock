void interrupt_service_routine(char pinval);
uint32_t interrupt_get_time_ms();
unsigned char interrupt_get_num_overflow();
void interrupt_read_dcf77_signal(unsigned char* val, unsigned char* ticks_ago);
