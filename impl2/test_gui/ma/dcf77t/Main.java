package ma.dcf77t;

import java.nio.file.Paths;
import java.io.IOException;

public class Main {

	public static void main(String[] args) throws IOException {
		LogTransferQueue log = new LogTransferQueue();
		VirtualDisplay disp = new VirtualDisplay(log);
		Subprocess proc = new Subprocess(Paths.get("..", "a.out"), log);
		proc.restart();
		SubprocessCommunicationProtocol proto =
				new SubprocessCommunicationProtocol(proc, disp);
		proto.start();
		AppWnd.createAndShow(disp, () -> {
			proto.interrupt();
			try {
				try {
					proc.close();
				} finally {
					proto.join();
				}
			} catch(IOException|InterruptedException ex) {
				ex.printStackTrace();
			} finally {
				System.exit(0);
			}
		}, log);
	}

}
