#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "vfd_gp9002.h"

/* ------------------------------------------- z Display Control Constants -- */
/*
 * Copied from https://github.com/adafruit/
 * Adafruit-Graphic-VFD-Display-Library/blob/master/Adafruit_GP9002.h
 */
#define GP9002_DISPLAYSOFF        0x00
#define GP9002_DISPLAY1ON         0x01
#define GP9002_DISPLAY2ON         0x02
#define GP9002_ADDRINCR           0x04
#define GP9002_ADDRHELD           0x05
#define GP9002_CLEARSCREEN        0x06
#define GP9002_CONTROLPOWER       0x07
#define GP9002_DATAWRITE          0x08
#define GP9002_DATAREAD           0x09
#define GP9002_LOWERADDR1         0x0A
#define GP9002_HIGHERADDR1        0x0B
#define GP9002_LOWERADDR2         0x0C
#define GP9002_HIGHERADDR2        0x0D
#define GP9002_ADDRL              0x0E
#define GP9002_ADDRH              0x0F
#define GP9002_OR                 0x10
#define GP9002_XOR                0x11
#define GP9002_AND                0x12
#define GP9002_BRIGHT             0x13 /* 3-5-10. Luminance Adjustment */
#define GP9002_DISPLAY            0x14
#define GP9002_DISPLAY_MONOCHROME 0x10
#define GP9002_DISPLAY_GRAYSCALE  0x14
#define GP9002_INTMODE            0x15
#define GP9002_DRAWCHAR           0x20
#define GP9002_CHARRAM            0x21
#define GP9002_CHARSIZE           0x22
#define GP9002_CHARBRIGHT         0x24

/* -------------------------------------------------- Forward Declarations -- */

static void write(char is_ctrl, char value);
static void writezero(unsigned addr, unsigned n);

/* -------------------------------------------------------- Implementation -- */

void vfd_gp9002_init(struct vfd_gp9002* ctx,
					enum vfd_gp9002_brightness brightness)
{
	unsigned char i;
	unsigned char initialization_sequence[][2] = {

		/* mode / initialization */
		{ 1, GP9002_DISPLAY },
		{ 0, GP9002_DISPLAY_MONOCHROME },

		/* brightness */
		{ 1, GP9002_BRIGHT },
		{ 0, brightness },

		/* 2nd screen starts at 0x400 */
		{ 1, GP9002_LOWERADDR2 },
		{ 0, 0x00 },
		{ 1, GP9002_HIGHERADDR2 },
		{ 0, 0x04 },

		/* clear and show vscreen 0 */
		{ 1, GP9002_CLEARSCREEN },
		{ 1, GP9002_DISPLAY1ON },

	};

	/* port directions */
	DDRB = (DDRB & ~_BV(DDB4)) /* IN */
		| _BV(DDB0) | _BV(DDB2) | _BV(DDB3) | _BV(DDB5) /* OUT */;

	/* hardware SPI */
	SPCR =  _BV(DORD) | /* data order lsb first */
		_BV(SPE)  | /* SPI enable */
		            /* not enabled: SPIE -- spi interrupt enable */
		_BV(MSTR) | /* SPI master mode */
		_BV(CPOL) | /* clock polarity: clock idle at 1 */
		_BV(CPHA) | /* clock phase: sample on leading edge of SCK */
		_BV(SPR0);  /* clock rate select: select f/16  */

	/* Uses /16 instead of /8 for frequency by unsetting SPI2X bit */
	SPSR &= ~_BV(SPI2X);

	PORTB |= _BV(VFD_GP9002_PIN_SS);

	for(i = 0; i < (sizeof(initialization_sequence) / (2 * sizeof(char)));
									i++)
		write(initialization_sequence[i][0],
						initialization_sequence[i][1]);

	ctx->lastfont = VFD_GP9002_FONT_NORMAL;
}

static void write(char is_ctrl, char value)
{
	PORTB = (PORTB & ~_BV(VFD_GP9002_PIN_CONTROL_DATA_INV)) |
				(is_ctrl << VFD_GP9002_PIN_CONTROL_DATA_INV);

	PORTB &= ~_BV(VFD_GP9002_PIN_SS);

	SPDR = value;
	while(!(SPSR & _BV(SPIF)))
		;

	PORTB |= _BV(VFD_GP9002_PIN_SS);
	_delay_us(1);

	if(is_ctrl && value == GP9002_CLEARSCREEN)
		_delay_us(270);
}

void vfd_gp9002_draw_string(struct vfd_gp9002* ctx,
		enum vfd_gp9002_vscreen vscreen, enum vfd_gp9002_font font,
		int y, int x, char clear, unsigned char len, const char* string)
{
	char fontw_raw;
	char fonth_raw;
	int  stringw;
	int  stringh;

	unsigned char clear_x;
	unsigned spos;

	/* font size */
	fontw_raw = (char)(font & 0xff);
	fonth_raw = (char)((font & 0xff00) >> 8);
	if(font != ctx->lastfont) {
		ctx->lastfont = font;
		write(1, GP9002_CHARSIZE);
		write(0, fontw_raw);
		write(0, fonth_raw);
	}

	/* clear area */
	if(clear) {
		stringw = len * (fontw_raw + 1) * 5;
		stringh =       (fonth_raw + 1) /* Do 8 at a time so no *8 */;

		/* Address = x * 8 + y / 8 */
		for(clear_x = 0; clear_x < stringw; clear_x++)
			writezero((clear_x + x) * 8 + y / 8 + (vscreen << 8),
								stringh);
	}

	/* target position */
	write(1, GP9002_CHARRAM);
	write(0, x + (vscreen == VFD_GP9002_VSCREEN_1? 128: 0));
	write(0, 0);
	write(0, y);

	/* send string */
	write(1, GP9002_DRAWCHAR);
	for(spos = 0; spos < len; spos++)
		write(0, string[spos]);
}

static void writezero(unsigned addr, unsigned n)
{
	unsigned i;

	write(1, GP9002_ADDRL);
	write(0, addr & 0xff);
	write(1, GP9002_ADDRH);
	write(0, (addr & 0xf00) >> 8);
	write(1, GP9002_DATAWRITE);
	for(i = 0; i < n; i++)
		write(0, 0x00);
}

void vfd_show_vscreen(struct vfd_gp9002* ctx, enum vfd_gp9002_vscreen vscreen)
{
	/*
	might work. If not try this (and then disable addr2 above...)

		{ 1, GP9002_LOWERADDR1 },
		{ 0, 0x00 },
		{ 1, GP9002_HIGHERADDR1 },
		{ 0, 0x04 }, -- vscreen0: 0, vscreen1: 0x04

	aalt. try write(1, GP9002_DISPLAYSOFF) before the ..on command.
	(would'nt flicker if we were to use it immediately after an interrupt
	but the interrupt functionality is not currently used/tested)
	*/
	switch(vscreen) {
	case VFD_GP9002_VSCREEN_0:
		write(1, GP9002_DISPLAY1ON);
		break;
	case VFD_GP9002_VSCREEN_1:
		write(1, GP9002_DISPLAY2ON);
		break;
	}
}

void vfd_clear(struct vfd_gp9002* ctx, enum vfd_gp9002_vscreen vscreen)
{
	/* Not sure if this is the best solution... */
	writezero(vscreen << 8, 0x400);
}
