package ma.dcf77t;

class UserIOStatus {

	// These variables are intended to be written from the GUI side and read
	// upon request from the protocol processor side.
	int wheel;
	int buttons;
	int light;

	// This variable is intended to be read from the GUI side and written
	// upon request from the protocol processor side.
	boolean buzzer;

}
