/* depends display.h, dcf77_timelayer.h */

struct ui {
	/* == private == */
	unsigned char al_h;
	unsigned char al_m;

	/* == public == */
	unsigned char in_sensor;
	unsigned char in_buttons;
	unsigned char in_mode;
	char          out_buzzer_on;
};

void ui_init(struct ui* ctx, struct display_shared* display);
void ui_update(struct ui* ctx, struct dcf77_bitlayer* bitlayer,
		struct dcf77_secondlayer* secondlayer,
		struct dcf77_timelayer* timelayer,
		struct display_shared* display, unsigned delay);
