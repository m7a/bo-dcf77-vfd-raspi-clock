struct display_ctx {
	/* internal to display.c */
	char vscreen;
	char brightness;
};

void display_init_ctx(struct display_ctx* ctx);
void display_update(struct display_ctx* ctx, struct display_shared* data);
