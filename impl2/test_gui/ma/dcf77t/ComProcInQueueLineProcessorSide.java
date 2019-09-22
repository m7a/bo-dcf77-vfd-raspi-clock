package ma.dcf77t;

@FunctionalInterface
interface ComProcInQueueLineProcessorSide {
	void sendLineToComProc(String line);
}
