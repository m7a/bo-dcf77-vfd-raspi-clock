# depends: avr-libc, avrdude, gcc-avr

OBJECTS = main.o input.o screen.o vfd_gp9002.o alarm.o interrupt.o \
		dcf77_low_level.o

%.o: %.c
	avr-gcc -Wall -O2 -DF_CPU=16000000UL -mmcu=atmega328p -c $*.c

build: $(OBJECTS)
	avr-gcc -mmcu=atmega328p $(OBJECTS) -o ma_dcf77_vfd
	avr-objcopy -O ihex -R .eeprom ma_dcf77_vfd ma_dcf77_vfd.hex

flash:
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 \
					-b 57600 -U flash:w:ma_dcf77_vfd.hex

clean:
	rm *.o ma_dcf77_vfd.hex ma_dcf77_vfd 2> /dev/null || true
