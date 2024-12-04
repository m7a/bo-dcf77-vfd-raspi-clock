test_secondlayer: test_secondlayer.adb ../src/dcf77_secondlayer.adb ../src/dcf77_secondlayer.ads
	gnatmake -Wall -I../src -o test_secondlayer test_secondlayer.adb

test_fuzz_qos9: test_fuzz_qos9.adb
	gnatmake -Wall --GCC=/usr/bin/afl-gcc-fast \
		-ftracer -fweb -ffunction-sections -p \
		-I../src_simulated -o test_fuzz_qos9 test_fuzz_qos9.adb

test_fuzz_qos9_normal: test_fuzz_qos9.adb
	gnatmake -g -Wall -I../src_simulated \
		-o test_fuzz_qos9_normal test_fuzz_qos9.adb

clean:
	-rm *.o *.ali "b~"*.ad?
