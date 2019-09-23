package ma.dcf77t;

class ComProcInMsg {

	static final ComProcInMsg MSG_TICK_0 = new ComProcInMsg(
					ComProcInMsgType.TICK, "0");

	static final ComProcInMsg MSG_TICK_1 = new ComProcInMsg(
					ComProcInMsgType.TICK, "1");

	static final ComProcInMsg MSG_DELAY_COMPLETED = new ComProcInMsg(
					ComProcInMsgType.DELAY_COMPLETED, null);

	final ComProcInMsgType type;
	final String           line;

	private ComProcInMsg(ComProcInMsgType type, String line) {
		this.type = type;
		this.line = line;
	}

	static ComProcInMsg createMsgLine(String line) {
		return new ComProcInMsg(ComProcInMsgType.LINE, line);
	}

}
