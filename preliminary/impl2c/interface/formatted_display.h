void formatted_display_coypright(struct display_shared* dshr);
void formatted_display_datetime(struct display_shared* dshr,
			short y, unsigned char m, unsigned char d,
			unsigned char h, unsigned char i, unsigned char s,
			unsigned char al_h, unsigned char al_m,
			char* info);
void formatted_display_debug_activity(struct display_shared* dshr,
			unsigned delay);
