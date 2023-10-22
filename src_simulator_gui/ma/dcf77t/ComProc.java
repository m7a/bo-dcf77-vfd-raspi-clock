package ma.dcf77t;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.function.Consumer;
import java.util.ArrayList;
import java.io.IOException;

class ComProc extends Thread {

	private final LinkedBlockingQueue<String> in;
	private final UserIOStatus                ustat;
	private final DCF77Sim                    sim;
	private final ComProcOutLine              outLine;
	private final SerialDisplayInterface      outSerial;
	private final Consumer<String>            outLog;

	private int wantAck = 0;

	ComProc(
		LinkedBlockingQueue<String> in,
		UserIOStatus                ustat,
		DCF77Sim                    sim,
		ComProcOutLine              outLine,
		SerialDisplayInterface      outSerial,
		Consumer<String>            outLog
	) {
		this.in        = in;
		this.ustat     = ustat;
		this.sim       = sim;
		this.outLine   = outLine;
		this.outSerial = outSerial;
		this.outLog    = outLog;
	}

	@Override
	public void run() {
		while(!isInterrupted()) {
			String msg;
			try {
				msg = in.take();
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

	private void procMsg(String msg) throws IOException {
		if(msg.startsWith("spi")) {
			handleSPI(msg);
		} else {
			int cidx = msg.indexOf(',');
			if (cidx == -1) {
				String ackPrefix = "ack," + msg + ",";
				String reply     = handleCall(msg);
				if (reply != null)
					outLine.writeLine(ackPrefix + reply);
			} else {
				handleCast(msg.substring(0, cidx),
						msg.substring(cidx + 1));
			}
		}
	}

	private void handleSPI(String msg) {
		String  type   = msg.substring(0, 5);
		boolean isCtrl = msg.charAt(6) == 'c' ? true : false;
		long    value  = Long.parseLong(msg.substring(8).trim());
		switch (type) {
		case "spi08": outSerial.accept08(isCtrl, value); break;
		case "spi16": outSerial.accept16(isCtrl, value); break;
		case "spi32": outSerial.accept32(isCtrl, value); break;
		default: outLog.accept("[ERROR   ] Unknown SPI type: " + type);
		}
	}

	private String handleCall(String msg) {
		switch (msg) {
		case "get_time_micros":
			return String.valueOf(System.nanoTime() / 1000);
		case "read_interrupt_signal":
			return readInterruptSignal();
		case "green_button_is_down":
			return bool2str(ustat.buttons.equals("green"));
		case "left_button_is_down":
			return bool2str(ustat.buttons.equals("left") ||
					ustat.buttons.equals("l+r"));
		case "right_button_is_down":
			return bool2str(ustat.buttons.equals("right") ||
					ustat.buttons.equals("l+r"));
		case "read_light_sensor":
			return String.valueOf(ustat.light);
		case "get_fault":
			return String.valueOf(sim.getFaults());
		default:
			outLog.accept("[ERROR   ] Unknown call: " + msg);
			return null;
		}
	}

	private String readInterruptSignal() {
		long[] iinfo = sim.getInterrupt();
		return (iinfo == null) ? "none" : (String.valueOf(iinfo[0]) +
						"," + String.valueOf(iinfo[1]));
	}

	private static String bool2str(boolean value) {
		return value ? "0" : "1";
	}

	private void handleCast(String type, String args) {
		switch (type) {
		case "set_buzzer_enabled":
			ustat.buzzer = str2bool(args);
			break;
		case "set_alarm_led_enabled":
			ustat.alarmLED = str2bool(args);
			break;
		case "log":
			outLog.accept("[ADALOG  ] " + args);
			break;
		default:
			outLog.accept("[ERROR   ] Unknown cast: " + type +
							" (args=" + args + ")");
		}
	}

	private static boolean str2bool(String value) {
		switch (value) {
		case "0": return false;
		case "1": return true;
		default:  throw new RuntimeException("Unknown bool value: " +
						value + ", expected 0 or 1");
		}
	}

}
