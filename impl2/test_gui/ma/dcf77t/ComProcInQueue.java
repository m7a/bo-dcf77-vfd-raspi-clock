package ma.dcf77t;

import java.util.concurrent.LinkedBlockingQueue;

class ComProcInQueue implements ComProcInQueueReceiverSide,
				ComProcInQueueTickerSide,
				ComProcInQueueLineProcessorSide {

	private final LinkedBlockingQueue<ComProcInMsg> q =
						new LinkedBlockingQueue<>();

	@Override
	public void sendDelayCompletedToComProc() {
		q.add(ComProcInMsg.MSG_DELAY_COMPLETED);
	}

	@Override
	public void sendTickToComProc(boolean isOneTick) {
		q.add(isOneTick? ComProcInMsg.MSG_TICK_1:
						ComProcInMsg.MSG_TICK_0);
	}

	@Override
	public void sendLineToComProc(String line) {
		q.add(ComProcInMsg.createMsgLine(line));
	}

	@Override
	public ComProcInMsg inComProcReceiveMsg() throws InterruptedException {
		return q.take();
	}

}
