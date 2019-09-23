DEPS = display.o interrupt.o dcf77_bitlayer.o

compile: $(DEPS)

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm $(DEPS) > /dev/null 2>&1 || true
