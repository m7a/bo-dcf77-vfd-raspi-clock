# compile for testing
#
# 	make compile-test -j
#
# compile for production
#
#	make compile-prod -j
#
# clean
#
# 	make clean

#MAIN        = test_display.o
#MAIN        = test_input.o
#MAIN        = test_alarm.o
MAIN        = main.o

CC_TEST     = gcc
CC_PROD     = avr-gcc
CFLAGS_DEBG = -Wall -I../interface -O0 -fprofile-arcs -ftest-coverage -DDEBUG
CFLAGS_TEST = -Wall -I../interface -O2 -fprofile-arcs -ftest-coverage -DTEST
CFLAGS_PROD = -Wall -I../interface -O2 -DF_CPU=16000000UL -mmcu=atmega328p
LDFLAGS_TEST = -lgcov --coverage

compile-test:
	make "CC=$(CC_TEST)" "CFLAGS=$(CFLAGS_TEST)" "LDFLAGS=$(LDFLAGS_TEST)" \
				"INNERDEP=c_ll_test c_test_auto" compile-inner

compile-debug:
	make "CC=$(CC_TEST)" "CFLAGS=$(CFLAGS_DEBG)" "LDFLAGS=$(LDFLAGS_TEST)" \
				"INNERDEP=c_ll_test c_test_auto" compile-inner

compile-prod:
	make "CC=$(CC_PROD)" "CFLAGS=$(CFLAGS_PROD)" INNERDEP=c_ll_hw \
								compile-inner

compile-inner: c_main c_src $(INNERDEP)
	gcc -o a.out ll_test/*.o src/*.o main/$(MAIN) $(LDFLAGS)

c_%:
	make -C $* CC=$(CC) "CFLAGS=$(CFLAGS)"

clean: cc_ll_test cc_test_auto cc_main cc_src cc_ll_hw
	cd test_gui && ant clean

cc_%:
	make -C $* CC=$(CC) "CFLAGS=$(CFLAGS)" clean

dist-clean: clean
	make -C test_auto dist-clean
	cd telegram_editor && ant dist-clean
	-rm a.out > /dev/null 2>&1
	-rm -r report_cov > /dev/null 2>&1

cov_%:
	make -C $* cov

cov: cov_ll_test cov_main cov_src cov_test_auto
	[ -d report_cov ] || mkdir report_cov
	covs=; for i in ll_test/cov.info main/cov.info src/cov.info \
			test_auto/cov.info; \
		do if [ -f "$$i" ]; then covs="$$covs $$i"; fi; done; \
			genhtml --output-directory report_cov $$covs
