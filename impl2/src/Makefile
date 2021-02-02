DEPS = display.o interrupt.o dcf77_bitlayer.o mainloop_timing.o \
	formatted_display.o dcf77_secondlayer_xeliminate.o \
	dcf77_secondlayer_moventries.o \
	dcf77_secondlayer_recompute_eom.o dcf77_secondlayer.o \
	dcf77_secondlayer_process_telegrams.o \
	dcf77_secondlayer_check_bcd_correct_telegram.o

compile: $(DEPS)

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	-rm $(DEPS) *.gcno *.gcda *.c.gcov cov.info > /dev/null 2>&1

cov:
	gcov *.c
	lcov -c -d . -o cov.info
