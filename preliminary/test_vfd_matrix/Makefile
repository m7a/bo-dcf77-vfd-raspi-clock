# depends: avr-libc, avrdude, gcc-avr

OBJECTS = main.o

%.o: %.c
	avr-gcc -Wall -O1 -DF_CPU=16000000UL -mmcu=atmega328p -c $*.c

build: $(OBJECTS)
	avr-gcc -mmcu=atmega328p $(OBJECTS) -o test_vfd_matrix
	avr-objcopy -O ihex -R .eeprom test_vfd_matrix test_vfd_matrix.hex

upload:
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 \
					-b 57600 -U flash:w:test_vfd_matrix.hex

dist-clean:
	rm test_vfd_matrix.hex test_vfd_matrix main.o 2>&1 || true
