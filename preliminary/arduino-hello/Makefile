# depends: avr-libc, avrdude, gcc-avr

build:
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o led.o led.c
	avr-gcc -mmcu=atmega328p led.o -o led
	avr-objcopy -O ihex -R .eeprom led led.hex

upload:
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 \
					-b 57600 -U flash:w:led.hex
