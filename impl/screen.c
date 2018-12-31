#include <stdio.h>
#include <string.h>

#include "vfd_gp9002.h"
#include "input.h"
#include "screen.h"

static void draw_init(struct screen* screen);
static void draw_time(struct screen* screen);
static void draw_date(struct screen* screen);
static void draw_measurements(struct screen* screen);

static const char* VERSION_INFO[] = {
/*       123456789ABCDEFGHIJKLMNOP */
	"Ma_Sys.ma VFD Clock v.",
	"1.0.0, (c)2018 Ma_Sys.ma.",
	"For further info send an"
	"email to Ma_Sys.ma@web.de",
/*       123456789ABCDEFGHIJKLMNOP */
};

void screen_init(struct screen* screen, struct vfd_gp9002* vfd)
{
	memset(screen, 0, sizeof(struct screen));

	screen->vfd          = vfd;
	screen->cur          = SCREEN_INITIAL;
	screen->displayed    = VFD_GP9002_VSCREEN_0;
	screen->next         = VFD_GP9002_VSCREEN_1;

	strcat(screen->time, "HH:ii:ss");
	screen->time_len = strlen(screen->time);
	strcat(screen->date, "DD.MM.YYYY");
	screen->date_len = strlen(screen->date);

	strcat(screen->alarm, "AH:AM");
	screen->alarm_len = strlen(screen->alarm);

	draw_init(screen);
	screen->enable_clear = 1;
}

static void draw_init(struct screen* screen)
{
	unsigned char y;
	if(screen->cur == SCREEN_INITIAL)
		for(y = 0; y < (sizeof(VERSION_INFO) / sizeof(char*)); y++)
			vfd_gp9002_draw_string(
				screen->vfd, screen->next,
				VFD_GP9002_FONT_NORMAL, y * 8, 0,
				screen->enable_clear, strlen(VERSION_INFO[y]),
				VERSION_INFO[y]
			);
}

void screen_set_time(struct screen* screen, char* time, unsigned char time_len)
{
	if(time_len > SCREEN_TIME_LEN) {
		memset(screen->time, 'T', SCREEN_TIME_LEN);
		screen->time_len = SCREEN_TIME_LEN;
	} else {
		memcpy(screen->time, time, time_len);
		screen->time_len = time_len;
	}
	draw_time(screen);
}

static void draw_time(struct screen* screen)
{
	switch(screen->cur) {
	case SCREEN_DATETIME:
	case SCREEN_DATETIME_ALARM:
		vfd_gp9002_draw_string(
			screen->vfd, screen->next,
			VFD_GP9002_FONT_TRIPLEW_QUADH, 8,
			/* center */
			128 - (screen->time_len * 3 * 5 / 2),
			screen->enable_clear, screen->time_len, screen->time
		);
		break;
	case SCREEN_STATUS:
		vfd_gp9002_draw_string(
			screen->vfd, screen->next, VFD_GP9002_FONT_NORMAL, 0, 0,
			screen->enable_clear, screen->time_len, screen->time
		);
		break;
	case SCREEN_INITIAL:
		/* ignore others */
		break;
	}
}

void screen_set_date(struct screen* screen, char* date, unsigned char date_len)
{
	if(date_len > SCREEN_DATE_LEN) {
		memset(screen->date, 'D', SCREEN_DATE_LEN);
		screen->date_len = SCREEN_DATE_LEN;
	} else {
		memcpy(screen->date, date, date_len);
		screen->date_len = date_len;
	}
	draw_date(screen);
}

static void draw_date(struct screen* screen)
{
	switch(screen->cur) {
	case SCREEN_DATETIME:
	case SCREEN_DATETIME_ALARM:
		vfd_gp9002_draw_string(
			screen->vfd, screen->next, VFD_GP9002_FONT_NORMAL, 40,
			/* center */
			128 - (screen->date_len * 5 / 2), screen->enable_clear,
			screen->date_len, screen->date
		);
		break;
	case SCREEN_STATUS:
		vfd_gp9002_draw_string(
			screen->vfd, screen->next,
			VFD_GP9002_FONT_NORMAL, (SCREEN_TIME_LEN + 1) * 5, 0,
			screen->enable_clear, screen->date_len, screen->date
		);
		break;
	case SCREEN_INITIAL:
		/* ignore others */
		break;
	}
}

void screen_set_measurements(struct screen* screen,
		unsigned char mode, unsigned char btn, unsigned char sensor,
		char mode_decoded, char btn_decoded)
{
	screen->mode         = mode;
	screen->mode_decoded = mode;
	screen->btn          = btn;
	screen->btn_decoded  = btn_decoded;
	screen->sensor       = sensor;

	draw_measurements(screen);
}

static void draw_measurements(struct screen* screen)
{
	char n;
	char meas_display[14];

	if(screen->cur == SCREEN_STATUS) {
		/* 3*00 + RMB + ,, + 2*0 + NUL = 14 */
		n = sprintf(meas_display, "R%02x,%02x,%02xM%dB%d", screen->mode,
				screen->btn, screen->sensor,
				screen->mode_decoded, screen->btn_decoded);
		vfd_gp9002_draw_string(
			screen->vfd, screen->next, VFD_GP9002_FONT_NORMAL,
			(SCREEN_DATE_LEN + SCREEN_TIME_LEN + 1) * 5, 0,
			screen->enable_clear, n, meas_display
		);
	}
}

void screen_update(struct screen* screen)
{
	enum vfd_gp9002_vscreen tmp;

	/* display */
	vfd_show_vscreen(screen->vfd, screen->next);

	/* exchange */
	tmp = screen->displayed;
	screen->displayed = screen->next;
	screen->next = tmp;
}

void screen_display(struct screen* screen, enum screen_id id)
{
	screen->cur = id;
	vfd_clear(screen->vfd, screen->next);
	screen->enable_clear = 0;
	draw_time(screen);
	draw_date(screen);
	draw_measurements(screen);
	screen->enable_clear = 1;
	screen_update(screen);

	/* provide an empty space for the next picture */
	vfd_clear(screen->vfd, screen->next);
}
