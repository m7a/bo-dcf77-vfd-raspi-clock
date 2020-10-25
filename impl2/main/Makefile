DEPS = test_display.o test_input.o test_alarm.o main.o

compile: $(DEPS)

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	-rm $(DEPS) *.gcno *.gcda *.c.gcov cov.info > /dev/null 2>&1

cov:
	gcov *.c
	lcov -c -d . -o cov.info
	if [ "$$(wc -c cov.info | cut -d" " -f1)" = 0 ]; then rm cov.info; fi
