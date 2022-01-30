package ma.dcf77t;

import java.util.function.Consumer;
import java.util.ArrayList;
import java.io.IOException;

class ComProc extends Thread {

	private final ComProcInQueueReceiverSide in;
	private final UserIOStatus               ustat;
	private final ComProcOutLine             outLine;
	private final ComProcOutRequestDelay     outRequestDelay;
	private final SerialDisplayInterface     outSerial;
	private final Consumer<String>           outLog;

	// private final ArrayList<String>          delayedLines;

	private int wantAck = 0;

	ComProc(
		ComProcInQueueReceiverSide in,
		UserIOStatus               ustat,
		ComProcOutLine             outLine,
		ComProcOutRequestDelay     outRequestDelay,
		SerialDisplayInterface     outSerial,
		Consumer<String>           outLog
	) {
		this.in              = in;
		this.ustat           = ustat;
		this.outLine         = outLine;
		this.outRequestDelay = outRequestDelay;
		this.outSerial       = outSerial;
		this.outLog          = outLog;
		//delayedLines         = new ArrayList<String>();
	}

	@Override
	public void run() {
		while(!isInterrupted()) {
			ComProcInMsg msg;
			try {
				msg = in.inComProcReceiveMsg();
			} catch(InterruptedException ex) {
				outLog.accept("[INFORMAT] ComProc shutdown " +
						"due to InterruptedException " +
						"(usually harmless)");
				return;
			}
			try {
				procMsg(msg);
			} catch(IOException ex) {
				outLog.accept("[WARNING ] ComProc.run: " + ex);
			}
		}
	}

	private void procMsg(ComProcInMsg msg) throws IOException {
		if(msg == ComProcInMsg.MSG_TICK_0) {
			outLine.writeLine("interrupt_service_routine,0");
			wantAck++;
		} else if(msg == ComProcInMsg.MSG_TICK_1) {
			outLine.writeLine("interrupt_service_routine,1");
			wantAck++;
		} else if(msg == ComProcInMsg.MSG_DELAY_COMPLETED) {
			outLine.writeLine("ACK,ll_delay_ms");
		} else {
			assert(msg.type == ComProcInMsgType.LINE);
			procLineMsg(msg.line);
		}
	}

	private void procLineMsg(String line) throws IOException {
		if(wantAck > 0) {
			if(line.equals("ACK,interrupt_service_routine")) {
				wantAck--;
				//processDelayedLines();
				return;
			} else {
				outLog.accept("[WARNING ] Mismatch: Expected " +
					"ACK,interrupt_service_routine but " +
					"got " + line); // + " ... delayed!");
				//delayedLines.add(line);
			}
		}
		//processDelayedLines();
		processRequestLine(line);
	}

/*
	private void processDelayedLines() throws IOException {
		if(delayedLines.size() != 0) {
			outLog.accept("[WARNING ] Resume processing " +
				delayedLines.size() + " delayed lines.");
			for(String delayedLine: delayedLines)
				processRequestLine(delayedLine);
			delayedLines.clear();
		}
	}
*/

	private void processRequestLine(String line) throws IOException {
		int idx = line.indexOf(',');
		if(idx == -1)
			procReadLine(line);
		else
			procKV(line.substring(0, idx), line.substring(idx + 1));
	}

	private void procReadLine(String line) throws IOException {
		switch(line) {	
		case "ll_input_read_sensor":
			outLine.writeLine(String.format(
				"REPL,ll_input_read_sensor,%02x", ustat.light));
			break;
		case "ll_input_read_buttons":
			outLine.writeLine(String.format(
				"REPL,ll_input_read_buttons,%02x",
				ustat.buttons));
			break;
		case "ll_input_read_mode":
			outLine.writeLine(String.format(
				"REPL,ll_input_read_mode,%02x", ustat.wheel));
			break;
		default:
			outLog.accept("[WARNING ] ComProc.procMsg: " +
						"unknown line: " + line);
		}
	}

	private void procKV(String key, String val) throws IOException {
		switch(key) {
		case "ll_delay_ms":
			outRequestDelay.accept(Integer.parseInt(val));
			break;
		case "ll_out_display":
			processDisplay(val);
			break;
		case "ll_out_buzzer":
			ustat.buzzer = val.equals("1");
			outLine.writeLine("ACK,ll_out_buzzer");
			break;
		case "ERROR":
			outLog.accept("[ERROR   ] ComProc.procKV: " + val);
			break;
		default:
			outLog.accept("[WARNING ] ComProc.procKV: " +
					"unknown KV: k=" + key + ",v=" + val);
			break;
		}
	}

	private void processDisplay(String val) throws IOException {
		try {
			int idx = val.indexOf(',');
			boolean ctrl = val.substring(0, idx).equals("1");
			int data = Integer.parseInt(val.substring(idx + 1));
			outSerial.accept(ctrl, data);
		} catch(RuntimeException ex) {
			outLog.accept("[WARNING ] ComProc.processDisplay: " +
								ex.toString());
		}
		// in any case: ack the message
		outLine.writeLine("ACK,ll_out_display");
	}

}
