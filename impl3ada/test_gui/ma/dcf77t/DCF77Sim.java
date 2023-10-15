package ma.dcf77t;

import java.util.ArrayList;
import java.util.List;
import java.io.*;

import static java.nio.charset.StandardCharsets.UTF_8;

/**
 * Part of the Ticker process, just keeps its own state separately.
 */
class DCF77Sim {

	private static final String RES_NAME = "testvector.csv";

	/** use 0,1,3 coding for 0: zero, 1: one, 3: no signal */
	private final int[][]  testVector;
	private final String[] testVectorComments;

	private int currentMinute = 0;
	private int currentSecond = 0;
	private int currentTicks  = 0;
	private int currentOnes   = 0;

	DCF77Sim() throws IOException {
		super();
		List<int[]>  minutes  = new ArrayList<>();
		List<String> comments = new ArrayList<>();
		try(BufferedReader rd = new BufferedReader(new InputStreamReader
				(getClass().getResourceAsStream(RES_NAME),
				UTF_8))) {
			String line;
			while((line = rd.readLine()) != null) {
				int sep = line.indexOf(';');
				char[] c = line.substring(0, sep).toCharArray();
				int[]  n = new int[c.length];
				for(int i = 0; i < c.length; i++)
					n[i] = (int)(c[i] - '0');
				minutes.add(n);
				comments.add(line.substring(sep + 1));
			}
		}
		testVector = minutes.toArray(new int[minutes.size()][]);
		testVectorComments = comments.toArray(new
						String[comments.size()]);
	}

	boolean tick() {
		if(++currentTicks >= (1000 / Ticker.MS_PER_TICK)) {
			currentTicks = 0;
			switch(nextSecond()) {
			case 0: currentOnes = 10; break;
			case 1: currentOnes = 25; break;
			case 3: currentOnes =  0; break;
			}
		}
		if(currentOnes <= 0) {
			return false;
		} else {
			currentOnes--;
			return true;
		}
	}

	private int nextSecond() {
		int rv = testVector[currentMinute][currentSecond++];
		if(currentSecond >= testVector[currentMinute].length) {
			currentSecond = 0;
			currentMinute++;
		}
		if(currentMinute >= testVector.length)
			currentMinute = 0; // wrap-around if data exceeded.
		return rv;
	}

}
