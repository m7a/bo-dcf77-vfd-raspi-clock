/*
 * Ma_Sys.ma DCF-77 VFD Module Clock 1.0.0, Copyright (c) 2018 Ma_Sys.ma.
 * For further info send an e-mail to Ma_Sys.ma@web.de.
 *
 * VFD Display Module Low Level Driver.
 * Does not expose all hardware functionality but a sensible choice for the
 * implementation task at hand.
 *
 * VScreens manage two different memory areas such that we can switch to the
 * new picture by means of a single command.
 *
 * Does not currently support flicker-free interrupt-based driving of the
 * display.
 */

/* Uses D10/SS, D11/MOSI, D12/MISO, D13/SCK and the following ports */
#define VFD_GP9002_PIN_CONTROL_DATA_INV PORTB0 /* D8  */
#define VFD_GP9002_PIN_SS               PORTB2 /* D10 */

/*
 * All brightness levels. Values correspond to the data to send with the
 * Luminance Adjustment command 0x13.
 */
enum vfd_gp9002_brightness {
	VFD_GP9002_BRIGHTNESS_PERC_100 = 0x00,
	VFD_GP9002_BRIGHTNESS_PERC_090 = 0x06,
	VFD_GP9002_BRIGHTNESS_PERC_080 = 0x0c,
	VFD_GP9002_BRIGHTNESS_PERC_070 = 0x12,
	VFD_GP9002_BRIGHTNESS_PERC_060 = 0x18,
	VFD_GP9002_BRIGHTNESS_PERC_050 = 0x1e,
	VFD_GP9002_BRIGHTNESS_PERC_040 = 0x24,
	VFD_GP9002_BRIGHTNESS_PERC_030 = 0x2a,
	VFD_GP9002_BRIGHTNESS_PERC_000 = 0xff,
};

enum vfd_gp9002_vscreen {
	VFD_GP9002_VSCREEN_0 = 0,
	VFD_GP9002_VSCREEN_1 = 4,
};

enum vfd_gp9002_font {
	VFD_GP9002_FONT_NORMAL        = 0x0000,
	VFD_GP9002_FONT_TRIPLEW_QUADH = 0x0203
};

struct vfd_gp9002_ctx {
	/* private: these variables are intended for internal usage only */
	enum vfd_gp9002_font lastfont;
};

void vfd_gp9002_init(struct vfd_gp9002* ctx,
					enum vfd_gp9002_brightness brightness);

/* For clear, y needs to be a multiple of 8 */
void vfd_gp9002_draw_string(struct vfd_gp9002* ctx,
		enum vfd_gp9002_vscreen vscreen, enum vfd_gp9002_font font,
		int y, int x, char clear, size_t len, char* string);

void vfd_show_vscreen(struct vfd_gp9002* ctx, enum vfd_gp9002_vscreen vscreen);

void vfd_clear(struct vfd_gp9002* ctx, enum vfd_gp9002_vscreen vscreen);
