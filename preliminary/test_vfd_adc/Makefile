# depends: avr-libc, avrdude, gcc-avr

OBJECTS = main.o

%.o: %.c
	avr-gcc -Wall -O1 -DF_CPU=16000000UL -mmcu=atmega328p -c $*.c

build: $(OBJECTS)
	avr-gcc -mmcu=atmega328p $(OBJECTS) -o test_vfd_adc
	avr-objcopy -O ihex -R .eeprom test_vfd_adc test_vfd_adc.hex

upload:
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 \
					-b 57600 -U flash:w:test_vfd_adc.hex

dist-clean:
	rm test_vfd_adc.hex test_vfd_adc main.o 2>&1 || true
