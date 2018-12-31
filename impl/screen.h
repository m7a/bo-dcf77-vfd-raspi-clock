enum screen_id {
	SCREEN_INITIAL         = 0,
	SCREEN_DATETIME        = 1,
	SCREEN_DATETIME_ALARM  = 2,
	SCREEN_STATUS          = 3,
};

#define SCREEN_TIME_LEN 8
#define SCREEN_DATE_LEN 10
#define SCREEN_ALARM_LEN 5

struct screen {
	/* private: internal structure, not intended for access from outside */
	struct vfd_gp9002* vfd;
	char enable_clear;
	enum vfd_gp9002_vscreen displayed;
	enum vfd_gp9002_vscreen next;
	enum screen_id cur;

	/* SCREEN 0: Initial (Version, Copyright) */

	/* SCREEN 1: Date, Time */
	char time_len;
	char time[SCREEN_TIME_LEN + 1];
	char date_len;
	char date[SCREEN_DATE_LEN + 1];

	/* SCREEN 2: Date, Time, Alarm Time TODO z NEEDS A SETTER... */
	char alarm_len;
	char alarm[SCREEN_ALARM_LEN + 1];

	/* SCREEN 3: Status */
	unsigned char mode;
	unsigned char mode_decoded;
	unsigned char btn;
	unsigned char btn_decoded;
	unsigned char sensor;

};

void screen_init(struct screen* screen, struct vfd_gp9002* vfd);

void screen_set_time(struct screen* screen, char* time, unsigned char time_len);
void screen_set_date(struct screen* screen, char* date, unsigned char date_len);
void screen_set_measurements(struct screen* screen, unsigned char mode,
					unsigned char btn, unsigned char sensor,
					char mode_decoded, char sensor_decoded);
void screen_update(struct screen* screen);
void screen_display(struct screen* screen, enum screen_id id);
