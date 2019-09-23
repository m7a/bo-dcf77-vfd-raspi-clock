DEPS = test_display.o test_input.o test_alarm.o

compile: $(DEPS)

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm $(DEPS) > /dev/null 2>&1 || true
