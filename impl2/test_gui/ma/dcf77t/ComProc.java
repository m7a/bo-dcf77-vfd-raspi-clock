package ma.dcf77t;

import java.io.IOException;

class ComProc extends Thread {

	private final ComProcInQueueReceiverSide in;
	private final ComProcOutLine             outLine;
	private final ComProcOutRequestDelay     outRequestDelay;
	private final SerialDisplayInterface     outSerial;

	private int wantAck = 0;

	ComProc(
		ComProcInQueueReceiverSide in,
		ComProcOutLine             outLine,
		ComProcOutRequestDelay     outRequestDelay,
		SerialDisplayInterface     outSerial
	) {
		this.in              = in;
		this.outLine         = outLine;
		this.outRequestDelay = outRequestDelay;
		this.outSerial       = outSerial;
	}

	@Override
	public void run() {
		while(!isInterrupted()) {
			ComProcInMsg msg;
			try {
				msg = in.inComProcReceiveMsg();
			} catch(InterruptedException ex) {
				ex.printStackTrace(); // harmless
				return;
			}
			try {
				procMsg(msg);
			} catch(IOException ex) {
				ex.printStackTrace(); // TODO report to user
			}
		}
	}

	private void procMsg(ComProcInMsg msg) throws IOException {
		if(msg == ComProcInMsg.MSG_TICK) {
			try {
				// TODO ATM IT IS ALWYS ZERO!
				outLine.writeLine(
					"interrupt_service_routine,0");
				wantAck++;
			} catch(IOException ex) {
				ex.printStackTrace(); // TODO z report
			}
		} else if(msg == ComProcInMsg.MSG_DELAY_COMPLETED) {
			outLine.writeLine("ACK,ll_delay_ms");
		} else {
			assert(msg.type == ComProcInMsgType.LINE);
			
			int idx = msg.line.indexOf(',');
			String key = msg.line.substring(0, idx);
			String val = msg.line.substring(idx + 1);

			if(wantAck > 0) {
				if(key.equals("ACK") && val.equals(
						"interrupt_service_routine")) {
					wantAck--;
					return;
				} else {
					System.out.println("[WARN] Mismatch: Expected ACK,interrupt_service_routine but got " + msg.line); // TODO PROPERLY REPORT TO USER
				}
			}

			switch(key) {
			case "ll_delay_ms":
				outRequestDelay.accept(Integer.parseInt(val));
				break;
			case "ll_out_display":
				// send this display request to the
				// display.
				processDisplay(val);
				break;
			case "ERROR":
				// TODO z something to report to the user
				System.out.println(msg.line);
				break;
			default:
				// TODO z also report but different error.
				System.out.println(msg.line);
				break;
			}
		}
	}

	private void processDisplay(String val) throws IOException {
		try {
			int idx = val.indexOf(',');
			boolean ctrl = val.substring(0, idx).equals("1");
			int data = Integer.parseInt(val.substring(idx + 1));
			outSerial.accept(ctrl, data);
		} catch(RuntimeException ex) {
			// TODO z report this to the user
			ex.printStackTrace();
		}
		// in any case: ack the message
		outLine.writeLine("ACK,ll_out_display");
	}

}
