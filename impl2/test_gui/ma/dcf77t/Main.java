package ma.dcf77t;

import java.nio.file.Paths;
import java.io.IOException;

public class Main {

	public static void main(String[] args) throws IOException {
		final UserIOStatus ustat = new UserIOStatus();
		final LogTransferQueue log = new LogTransferQueue();
		final ComProcInQueue comIn = new ComProcInQueue();
		final VirtualDisplaySPI disp = new VirtualDisplaySPI(log);
		final Subprocess proc = new Subprocess(Paths.get("..", "a.out"),
								log, comIn);
		final Ticker tick = new Ticker(comIn);
		final ComProc proto = new ComProc(comIn, ustat, proc, tick,
								disp, log);

		proc.restart(); // start subprocess
		proc.start();   // start thread
		proto.start();
		tick.start();

		AppWnd.createAndShow(new VirtualDisplay(disp), () -> {
			// Shutdown routine (welcome to the world of JS)
			tick.interrupt();
			proto.interrupt();
			proc.interrupt();
			ChainedTries.tryThem(
				() -> proc.close(),
				() -> tick.join(),
				() -> proto.join(),
				() -> proc.join()
			);
			System.exit(0);
		}, log, ustat);
	}

}
