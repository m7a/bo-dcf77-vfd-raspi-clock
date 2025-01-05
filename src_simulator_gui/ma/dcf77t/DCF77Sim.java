package ma.dcf77t;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicIntegerArray;
import java.util.concurrent.atomic.AtomicInteger;
import static java.nio.charset.StandardCharsets.UTF_8;

class DCF77Sim {

	private static final String RES_NAME = "testvector.csv";

	private static final long SECOND        = 1_000_000_000;
	private static final long DEFAULT_SHIFT =    20_000_000; // 20ms

	// -- Special encoding of seconds --
	// Bit 0..1: 00 - BIT 0
	//           01 - BIT 1
	//           10 - no value / no update (leading padding)
	//           11 - NO SIGNAL (end of minute marker or nothing received)
	private static final int VALUE_MASK      = 0x03;
	private static final int VALUE_BIT_0     = 0x00;
	private static final int VALUE_BIT_1     = 0x01;
	private static final int VALUE_NONE      = 0x02;
	private static final int VALUE_NO_SIGNAL = 0x03;
	// Bit 2:     0 - valid
	//            1 - force to invalid (NO_SIGNAL output instead of value)
	private static final int EDIT_MASK          = 0x04;
	private static final int EDIT_VALID         = 0x00;
	private static final int EDIT_FORCE_INVALID = 0x04;
	// Bit 3..4: 00 - not processed yet
	//           01 - missed
	//           11 - processed OK
	private static final int PROCESSING_STATE_MASK   = 0x18;
	private static final int PROCESSING_STATE_NONE   = 0x00;
	private static final int PROCESSING_STATE_MISSED = 0x08;
	private static final int PROCESSING_STATE_OK     = 0x18;
	// -- End Special encoding of seconds --

	private final AtomicIntegerArray[] testVectorMinutes;
	private final String[]             testVectorComments;

	private final AtomicInteger faults = new AtomicInteger(0);
	private int[] lastMinSec = { 0, 0 }; // deprecated
	private long  lastQuery  = -1;
	private long  lastBegin  = 0; // deprecated
	private long  refStart   = 0;

	DCF77Sim() throws IOException {
		super();
		List<AtomicIntegerArray> minutes  = new ArrayList<>();
		List<String>             comments = new ArrayList<>();
		try(BufferedReader rd = new BufferedReader(new InputStreamReader
				(getClass().getResourceAsStream(RES_NAME),
				UTF_8))) {
			String line;
			while((line = rd.readLine()) != null) {
				int sep = line.indexOf(';');
				char[] c = line.substring(0, sep).toCharArray();
				AtomicIntegerArray n = new AtomicIntegerArray(
								c.length);
				for(int i = 0; i < c.length; i++)
					n.set(i, (int)(c[i] - '0'));
				minutes.add(n);
				comments.add(line.substring(sep + 1));
			}
		}
		testVectorMinutes = minutes.toArray(
					new AtomicIntegerArray[minutes.size()]);
		testVectorComments = comments.toArray(new
					String[comments.size()]);
	}

	/**
	 * Note that this implementation does not currently maintain the
	 * processing state that was originally intended to be maintained...
	 */
	long nextBits() {
		long now = System.nanoTime() / 1_000; // all in µs
		if (refStart <= 0) {
			refStart = now;
			lastQuery = now - 100_000;
		}

		int bitsRequested = (int)((now - lastQuery) / 7_000);
		lastQuery = now;
		if (bitsRequested > 29) {
			// requested too much
			faults.incrementAndGet(); // don't care about RV
			bitsRequested = 29;
		}

		int secondComplete = (int)((now - refStart) / 1_000_000);

		byte[] windowData = new byte[286];
		int nowOffset = 143;
		for (int i = -1; i <= 0; i++) {
			int secondToRequest = secondComplete + i;
			int secondOfMinute = secondToRequest % 60;
			int minute = secondToRequest / 60;
			int valm;

			if (secondToRequest < 0 ||
			minute >= testVectorMinutes.length ||
			secondOfMinute >= testVectorMinutes[minute].length() ||
			((valm = testVectorMinutes[minute].get(secondOfMinute))
			& EDIT_MASK) == EDIT_FORCE_INVALID) {
				for (int j = 0; j < 143; j++)
					windowData[nowOffset + i * nowOffset +
									j] = 0;
				continue;
			}

			int setTo1 = 0;
			switch (valm & VALUE_MASK) {
			case VALUE_BIT_0: setTo1 = 14; break;
			case VALUE_BIT_1: setTo1 = 28; break;
			}

			for (int j = 0; j < 143; j++)
				windowData[nowOffset + i * nowOffset + j] =
					(j < setTo1) ? (byte)1 : (byte)0;
		}

		int timingInSecond =
			(int)(((now - refStart) % 1_000_000) * 143 / 1_000_000);

		//char infoBits[] = new char[bitsRequested + 1];
		//infoBits[0] = '1';

		long rv = 1;
		for (int i = -bitsRequested + 1; i <= 0; i++) {
			rv = (rv << 1) | windowData[nowOffset +
							timingInSecond + i];
			//infoBits[bitsRequested + i] = windowData[nowOffset +
			//		timingInSecond + i] == 0 ? '0' : '1';
		}

		//System.out.println(new String(infoBits));

		return rv;
	}

	int getFaults() {
		return faults.get();
	}

// -----------------------------------------------------------------------------
// DEPRECATED - IF A MODEL DECIDES TO CONSISTENTLY ONLY USE THE OLD STYLE IT MAY
//              STILL WORK THUS NOT DELETED FOR NOW
// -----------------------------------------------------------------------------

	private int getMinutes() {
		return testVectorMinutes.length;
	}

	private int getSeconds(int minute) {
		return testVectorMinutes[minute].length();
	}

	private int getSecond(int... minSec) {
		return testVectorMinutes[minSec[0]].get(minSec[1]);
	}

	/** @return null for not found */
	private int[] findUnprocessed(int... start) {
		if (start == null)
			return null;

		int j = start[1];
		for (int i = start[0]; i < getMinutes(); i++) {
			for (; j < getSeconds(i); j++) {
				int sec = getSecond(i, j);
				if ((sec & PROCESSING_STATE_MASK) ==
							PROCESSING_STATE_NONE &&
						(sec & VALUE_MASK) !=
							VALUE_NONE)
					return new int[] { i, j };
			}
			j = 0;
		}

		return null; // nothing found
	}

	/** @return null for “no update” */
	long[] getInterrupt() {
		long now = System.nanoTime();
		if (lastQuery == -1) {
			// initial case
			lastQuery  = now;
			lastMinSec = findUnprocessed(lastMinSec);
			lastBegin  = now - SECOND + DEFAULT_SHIFT;
			return toInterruptValue();
		} else if (now > lastQuery + SECOND) {
			// if outside of timeframe!
			while (now > (lastQuery + 2 * SECOND)) {
				lastMinSec = findUnprocessed(lastMinSec);
				if (lastMinSec == null)
					return null; // cancel, nothing to do
				int val = getSecond(lastMinSec);
				testVectorMinutes[lastMinSec[0]].set(
					lastMinSec[1],
					(val & ~PROCESSING_STATE_MASK) |
					PROCESSING_STATE_MISSED);
				lastQuery += SECOND;
				lastBegin += SECOND;
				faults.incrementAndGet(); // don't care about RV
			}
			lastQuery  = now;
			lastMinSec = findUnprocessed(lastMinSec);
			lastBegin += SECOND;
			return toInterruptValue();
		} else {
			return null; // asked too early -> no update
		}
	}

	private long[] toInterruptValue() {
		if (lastMinSec == null)
			return null;

		int val = getSecond(lastMinSec);
		testVectorMinutes[lastMinSec[0]].set(lastMinSec[1],
			(val & ~PROCESSING_STATE_MASK) | PROCESSING_STATE_OK);

		int valm = val & VALUE_MASK;
		if ((val & EDIT_MASK) == EDIT_FORCE_INVALID)
			// no signal is like no update here
			return null;

		switch (valm) {
		case VALUE_BIT_0:
			// all output has to be in µs
			return new long [] { 100_000, lastBegin / 1_000 };
		case VALUE_BIT_1:
			return new long [] { 200_000, lastBegin / 1_000 };
		case VALUE_NO_SIGNAL:
			return null;
		default:
			throw new RuntimeException("Unexpected value: " + valm +
					" (val=" + val + ", lastMinSec=" +
					lastMinSec + ")");
		}
	}

}
