package ma.dcf77t;

@FunctionalInterface
interface ComProcInQueueReceiverSide {
	ComProcInMsg inComProcReceiveMsg() throws InterruptedException;
}
