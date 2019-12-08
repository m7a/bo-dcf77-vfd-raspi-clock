DEPS = test_auto_xeliminate.o test_auto_secondlayer.o decode_telegram_bin.o \
	test_auto_moventries.o

compile: $(DEPS)
	$(CC) $(CFLAGS) -o test_auto_xeliminate test_auto_xeliminate.o \
						../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) -o test_auto_moventries test_auto_moventries.o \
						../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) -o test_auto_secondlayer test_auto_secondlayer.o \
						../ll_test/*.o ../src/*.o
	$(CC) $(CFLAGS) -o decode_telegram_bin decode_telegram_bin.o \
						../ll_test/*.o ../src/*.o

%.o: %.c ../interface/%.h
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm $(DEPS) > /dev/null 2>&1 || true

dist-clean: clean
	rm test_auto_xeliminate test_auto_secondlayer decode_telegram_bin \
				test_auto_moventries > /dev/null 2>&1 || true
