package ma.dcf77t;

import java.nio.file.Paths;
import java.io.IOException;

public class Main {

	public static void main(String[] args) throws IOException {
		final LogTransferQueue log = new LogTransferQueue();
		final ComProcInQueue comIn = new ComProcInQueue();
		final VirtualDisplay disp = new VirtualDisplay(log);
		final Subprocess proc = new Subprocess(Paths.get("..", "a.out"),
								log, comIn);
		final Ticker tick = new Ticker(comIn);
		final ComProc proto = new ComProc(comIn, proc, tick, disp);

		proc.restart(); // start subprocess
		proc.start();   // start thread
		proto.start();
		tick.start();
		
		AppWnd.createAndShow(disp, () -> {
			tick.interrupt();
			proto.interrupt();
			proc.interrupt();
			try {
				try {
					proc.close();
				} finally {
					try {
						tick.join();
					} finally {
						try {
							proto.join();
						} finally {
							proc.join();
						}
					}
				}
			} catch(IOException|InterruptedException ex) {
				ex.printStackTrace();
			} finally {
				System.exit(0);
			}
		}, log);
	}

}
