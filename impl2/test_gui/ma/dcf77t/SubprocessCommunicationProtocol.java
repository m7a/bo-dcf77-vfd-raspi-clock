package ma.dcf77t;

import java.io.IOException;
import java.util.function.Consumer;

class SubprocessCommunicationProtocol extends Thread {

	// TODO CSTAT PROBLEM: TICKER DELAY() is in progress while ACKs are received but they are not read from the java side because the java side hangs at the delay. Now we should possibly solve this by establishing a callback system between Ticker and subprocess com protocol. Actually this interrelates the two of them so much that it begins to make sense to incorporate them into the same unit? / The circular dependency already hints this. Somehow one would then like to extract the actual functionality to a separate unit s.t. the protocol only does ticking + delay + syncing issues? -- better design needed! -> substat deprecate in favour of a ComProc + new ticker implementation with the defined queues! + add an stdin drainer thread... | we have stdin drainer + ticker now replace this messy subprocess com protocol !

	private final Subprocess proc;
	private final SerialDisplayInterface disp;

	private Consumer<Integer> tickerDelayFunc;
	private int wantAck = 0;

	SubprocessCommunicationProtocol(Subprocess proc,
						SerialDisplayInterface disp) {
		super("SubprocessCommunicationProtocol");
		this.proc = proc;
		this.disp = disp;
	}

	void setTickerDelayFunc(Consumer<Integer> tickerDelayFunc) {
		this.tickerDelayFunc = tickerDelayFunc;
	}

	@Override
	public void run() {
		while(!isInterrupted()) {
			try {
				String line = proc.readLine();
				if(line == null) {
					// oh no, subprocess has terminated.
					// This is quite unfortuante and means it
					// should probably be restarted. However, we
					// will possibly leave that at the discretion
					// of the user.
					// TODO z signal process activity somehow, for now we only go to sleep...
					try {
						sleep(100);
					} catch(InterruptedException ex) {
						// now the thread was even interrupted.
						// Time for program exit...
						ex.printStackTrace(); // harmless
						return;
					}
				} else {
					processLine(line);
				}
			} catch(IOException ex) {
				ex.printStackTrace(); // TODO PROPERLY REPORT TO USER
			}
		}
	}

	private void processLine(String line) throws IOException {
		// Protocol operates with CSV
		int idx = line.indexOf(',');
		String key = line.substring(0, idx);
		String val = line.substring(idx + 1);

		if(wantAck > 0) {
			if(key.equals("ACK") && val.equals(
						"interrupt_service_routine")) {
				synchronized(this) {
					wantAck--;
				}
				return;
			} else {
				System.out.println("[WARN] Mismatch: Expected ACK,interrupt_service_routine but got " + line); // TODO PROPERLY REPORT TO USER
			}
		}

		switch(key) {
		case "ll_delay_ms":
			if(tickerDelayFunc == null)
				System.out.println(
					"[WARN] tickerDelayFunc not set.");
			else
				tickerDelayFunc.accept(Integer.parseInt(val));
			break;
		case "ll_out_display":
			// send this display request to the
			// display.
			processDisplay(val);
			break;
		case "ERROR":
			// TODO z something to report to the user
			System.out.println(line);
			break;
		default:
			// TODO z also report but different error.
			System.out.println(line);
			break;
		}
	}

	private void processDisplay(String val) throws IOException {
		try {
			int idx = val.indexOf(',');
			boolean ctrl = val.substring(0, idx).equals("1");
			int data = Integer.parseInt(val.substring(idx + 1));
			disp.accept(ctrl, data);
		} catch(RuntimeException ex) {
			// TODO z report this to the user
			ex.printStackTrace();
		}

		// in any case: ack the message
		synchronized(this) {
			proc.writeLine("ACK,ll_out_display");
		}
	}

	synchronized void handleTickInterrupt() {
		try {
			proc.writeLine("interrupt_service_routine,0");
			wantAck++;
		} catch(IOException ex) {
			ex.printStackTrace(); // TODO z Report
		}
	}

}
