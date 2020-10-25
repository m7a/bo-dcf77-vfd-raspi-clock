DEPS = display.o interrupt.o dcf77_bitlayer.o mainloop_timing.o \
	formatted_display.o dcf77_proc_xeliminate.o dcf77_proc_moventries.o \
	dcf77_secondlayer.o

compile: $(DEPS)

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	-rm $(DEPS) *.gcno *.gcda *.c.gcov cov.info > /dev/null 2>&1

cov:
	gcov *.c
	lcov -c -d . -o cov.info
