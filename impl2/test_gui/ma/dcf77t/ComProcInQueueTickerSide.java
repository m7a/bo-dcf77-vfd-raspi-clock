package ma.dcf77t;

interface ComProcInQueueTickerSide {
	void sendDelayCompletedToComProc();
	void sendTickToComProc();
}
