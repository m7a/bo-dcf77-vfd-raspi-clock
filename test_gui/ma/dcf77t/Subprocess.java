package ma.dcf77t;

import java.io.*;
import java.nio.file.Path;
import java.util.function.Consumer;
import java.util.concurrent.LinkedBlockingQueue;

import static java.nio.charset.StandardCharsets.UTF_8;

/** Derived from ma.d5man.lib.export.proc.AbstractConverterProcess */
class Subprocess extends Thread implements AutoCloseable, ComProcOutLine {

	private final long bootTime = System.currentTimeMillis();

	private final String executable;
	private final Consumer<String> log;
	private final LinkedBlockingQueue<String> lineStream;

	private Process        proc;
	private Writer         stdin;
	private BufferedReader stdout;

	private long lastRead = 0;

	Subprocess(Path executable, Consumer<String> log,
				LinkedBlockingQueue<String> lineStream) {
		this.executable = executable.toAbsolutePath().toString();
		this.log        = log;
		this.lineStream = lineStream;
	}

	@Override
	public void run() {
		String line = null;
		try {
			while(!isInterrupted() &&
					(line = stdout.readLine()) != null) {
				lastRead = System.currentTimeMillis();
				// hide SPI traffic for now
				if (!line.startsWith("spi"))
					log.accept(String.format("[%8d] %s",
							deltaT(), line));
				lineStream.add(line);
			}
		} catch(IOException ex) {
			log.accept("[ERROR   ] Subprocess terminated due to " +
							"IOException: " + ex);
		}
		if(line == null)
			log.accept("[WARNING ] Subporcess EOF.");
	}

	/** also call this in the very beginning */
	void restart() throws IOException {
		try {
			if(proc != null)
				close();
		} finally {
			ProcessBuilder pb = new ProcessBuilder(executable);
			// just send stderr to console as we do not want
			// to use it really.
			pb.redirectError(ProcessBuilder.Redirect.INHERIT);
			proc   = pb.start();
			stdin  = new OutputStreamWriter(
						proc.getOutputStream(), UTF_8);
			stdout = new BufferedReader(new InputStreamReader(
						proc.getInputStream(), UTF_8));
		}
	}

	@Override
	public void writeLine(String str) throws IOException {
		/*
		while((System.currentTimeMillis() - lastRead) < 5) {
			try { Thread.sleep(5); } catch(InterruptedException ex) {}
		}
		*/
		log.accept(String.format("[%8d] > %s", deltaT(), str));
		stdin.write(str);
		stdin.write('\n');
		stdin.flush();
	}

	private long deltaT() {
		return System.currentTimeMillis() - bootTime;
	}

	@Override
	public void close() throws IOException {
		// if restart has never been called, but suprocess was
		// instantiated as part of a try with resources, one would get
		// null pointer exceptions without this.
		if(proc == null)
			return;

		// Nested construction: Try all of these things, even if some
		// fail...
		Exception es = ChainedTries.tryThem(
			() -> stdin.close(),
			() -> stdout.close(),
			() -> {
				try {
					proc.waitFor();
				} catch(Exception ex) {
					proc.destroy();
					throw new IOException(
						"Failed to wait for process " +
						"termination.", ex
					);
				}
			},
			() -> { proc = null; }
		);

		// just throw the first one, the remainder is already logged
		if(es != null)
			throw new IOException(es);
	}

}
