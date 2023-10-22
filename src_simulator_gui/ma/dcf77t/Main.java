package ma.dcf77t;

import java.util.concurrent.LinkedBlockingQueue;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.io.IOException;

public class Main {

	private static final Path EXECUTABLE = Paths.get("..", "src_simulated",
								"dcf77vfd");

	public static void main(String[] args) throws IOException {

		final LinkedBlockingQueue<String>
					comIn = new LinkedBlockingQueue<>();
		final UserIOStatus      ustat = new UserIOStatus();
		final LogTransferQueue  log   = new LogTransferQueue();
		final DCF77Sim          sim   = new DCF77Sim();
		final VirtualDisplaySPI disp  = new VirtualDisplaySPI(log);
		final Subprocess        proc  = new Subprocess(
							EXECUTABLE, log, comIn);
		final ComProc           proto = new ComProc(comIn, ustat, sim,
							proc, disp, log);

		proc.restart(); // start subprocess
		proc.start();   // start thread
		proto.start();

		AppWnd.createAndShow(new VirtualDisplay(disp), () -> {
			// Shutdown routine
			proto.interrupt();
			proc.interrupt();
			ChainedTries.tryThem(
				() -> proc.close(),
				() -> proto.join(),
				() -> proc.join()
			);
			System.exit(0);
		}, log, ustat);
	}

}
