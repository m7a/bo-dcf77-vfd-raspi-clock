DEPS = display.o interrupt.o dcf77_bitlayer.o mainloop_timing.o \
	formatted_display.o dcf77_proc_xeliminate.o

compile: $(DEPS)

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm $(DEPS) > /dev/null 2>&1 || true
