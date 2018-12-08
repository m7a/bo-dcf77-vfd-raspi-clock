# depends: avr-libc, avrdude, gcc-avr

OBJECTS = main.o task_heating_wire.o 

%.o: %.c
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c $*.c

build: $(OBJECTS)
	avr-gcc -mmcu=atmega328p $(OBJECTS) -o ma_dcf77
	avr-objcopy -O ihex -R .eeprom ma_dcf77 ma_dcf77.hex

upload:
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 \
					-b 57600 -U flash:w:ma_dcf77.hex
