package ma.dcf77t;

interface SerialDisplayInterface {

	void accept08(boolean isCtrl, long data);
	void accept16(boolean isCtrl, long data);
	void accept32(boolean isCtrl, long data);

}
