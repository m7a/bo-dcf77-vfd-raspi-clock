DEPS = ll_delay.o ll_out_display.o ll_interrupt.o \
	ll_test_acklogic_and_interrupt.o ll_input.o ll_out_buzzer.o

compile: $(DEPS)
	
%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm $(DEPS) > /dev/null 2>&1 || true
