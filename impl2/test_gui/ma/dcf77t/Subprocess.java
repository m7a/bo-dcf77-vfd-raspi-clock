package ma.dcf77t;

import java.io.*;
import java.nio.file.Path;
import java.util.function.Consumer;

import static java.nio.charset.StandardCharsets.UTF_8;

/** Derived from ma.d5man.lib.export.proc.AbstractConverterProcess */
class Subprocess implements AutoCloseable {

	private final String executable;
	private final Consumer<String> log;

	private Process        proc;
	private Writer         stdin;
	private BufferedReader stdout;

	Subprocess(Path executable, Consumer<String> log) {
		this.executable = executable.toAbsolutePath().toString();
		this.log = log;
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

	void writeLine(String str) throws IOException {
		log.accept("> " + str);
		stdin.write(str);
		stdin.write('\n');
		stdin.flush();
	}

	String readLine() throws IOException {
		String line = stdout.readLine();
		log.accept(line == null? "[WARN] Subprocess EOF": line);
		return line;
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
		try {
			stdin.close();
		} finally {
			try {
				stdout.close();
			} finally {
				try {
					proc.waitFor();
				} catch(Exception ex) {
					proc.destroy();
					throw new IOException(
						"Failed to wait for process " +
						"termination.", ex
					);
				} finally {
					proc = null;
				}
			}
		}
	}

}
