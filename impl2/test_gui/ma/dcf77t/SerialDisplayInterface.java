package ma.dcf77t;

interface SerialDisplayInterface {

	/* @param data is an unsigned byte 0..255 */
	void write(boolean isCtrl, int data);

}
