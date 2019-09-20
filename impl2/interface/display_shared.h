#define DISPLAY_MAX_NUM_ITEMS 8
#define DISPLAY_MEMORY_SIZE   256

#define DISPLAY_FONT_NORMAL 0
#define DISPLAY_FONT_LARGE  1

enum vfd_gp9002_brightness {
	DISPLAY_BRIGHTNESS_PERC_100 = 0x00,
	DISPLAY_BRIGHTNESS_PERC_090 = 0x06,
	DISPLAY_BRIGHTNESS_PERC_080 = 0x0c,
	DISPLAY_BRIGHTNESS_PERC_070 = 0x12,
	DISPLAY_BRIGHTNESS_PERC_060 = 0x18,
	DISPLAY_BRIGHTNESS_PERC_050 = 0x1e,
	DISPLAY_BRIGHTNESS_PERC_040 = 0x24,
	DISPLAY_BRIGHTNESS_PERC_030 = 0x2a,
	DISPLAY_BRIGHTNESS_PERC_000 = 0xff,
};

struct display_shared {

	unsigned char set_brightness;
	unsigned char num_entries;
	unsigned char entry_lengths[DISPLAY_MAX_NUM_ITEMS];
	unsigned char entry_offsets[DISPLAY_MAX_NUM_ITEMS];
	unsigned char entry_x      [DISPLAY_MAX_NUM_ITEMS];
	unsigned char entry_y      [DISPLAY_MAX_NUM_ITEMS];
	unsigned char entry_font   [DISPLAY_MAX_NUM_ITEMS];
	         char entry_text   [DISPLAY_MEMORY_SIZE];

};
