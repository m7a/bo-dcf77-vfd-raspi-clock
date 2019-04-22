test-dcf77-high-level-compile: test_dcf77_high_level.c
	gcc -Wall -I ../interface -o test_dcf77_high_level \
							test_dcf77_high_level.c

decode_telegram_bin: decode_telegram_bin.c
	gcc -o decode_telegram_bin decode_telegram_bin.c 

test-dcf77-low-level-compile: test_dcf77_low_level.c
	gcc -I ../interface -o test_dcf77_low_level ../src/dcf77_low_level.c \
							test_dcf77_low_level.c

dist-clean:
	-rm test_dcf77_low_level test_dcf77_high_level decode_telegram_bin
