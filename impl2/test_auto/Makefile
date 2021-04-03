DEPS = test_auto_xeliminate.o test_auto_secondlayer.o decode_telegram_bin.o \
	test_auto_moventries.o test_auto_checkbcd.o

compile: $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o test_auto_xeliminate \
			test_auto_xeliminate.o ../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o test_auto_moventries \
			test_auto_moventries.o ../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o test_auto_secondlayer \
			test_auto_secondlayer.o ../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o test_auto_checkbcd \
			test_auto_checkbcd.o ../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o decode_telegram_bin \
			decode_telegram_bin.o ../ll_test/*.o ../src/*.o

test_auto_xeliminate.c: xeliminate_testcases.h

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	-rm $(DEPS) *.gcno *.gcda *.c.gcov cov.info > /dev/null 2>&1

cov:
	gcov *.c
	lcov -c -d . -o cov.info

dist-clean: clean
	rm test_auto_xeliminate test_auto_secondlayer decode_telegram_bin \
		test_auto_moventries test_auto_checkbcd > /dev/null 2>&1 || true
