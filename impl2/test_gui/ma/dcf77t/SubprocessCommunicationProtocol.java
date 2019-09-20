package ma.dcf77t;

import java.io.IOException;

class SubprocessCommunicationProtocol extends Thread {

	private final Subprocess proc;
	private final SerialDisplayInterface disp;

	SubprocessCommunicationProtocol(Subprocess proc,
						SerialDisplayInterface disp) {
		super("SubprocessCommunicationProtocol");
		this.proc = proc;
		this.disp = disp;
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
		switch(key) {
		case "ll_delay_ms":
			// TODO USE TICKER
			System.out.println(line);
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
		proc.writeLine("ACK,ll_out_display");
	}

}
