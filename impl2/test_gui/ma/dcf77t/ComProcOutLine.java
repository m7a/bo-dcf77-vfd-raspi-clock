package ma.dcf77t;

import java.io.IOException;

@FunctionalInterface
interface ComProcOutLine {
	void writeLine(String str) throws IOException;
}
