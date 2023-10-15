package ma.dcf77t;

import java.util.function.Consumer;

/**
 * Implement the protocol for the display without exposing everything to
 * VirutalDisplay which uses this class.
 */
class VirtualDisplaySPI implements SerialDisplayInterface {

	private final Consumer<String> log;

	private int[]       memory              = new int[0x909];
	private boolean[]   screenOn            = new boolean[] { false, false };
	private int[]       lowerScreenAddress  = new int[] { 0, 0 };
	private int[]       higherScreenAddress = new int[] { 0, 0 };

	private int         lowerRWAddress      = 0;
	private int         higherRWAddress     = 0;
	private int         currentAddress      = 0; /* auto increment */
	private DisplayCtrl lastCtrl            = DisplayCtrl.NONE;

	VirtualDisplaySPI(Consumer<String> log) {
		this.log = log;
	}

	@Override
	public void accept08(boolean isCtrl, long data) {
		accept(isCtrl, (int) data);
	}

	public void accept(boolean isCtrl, int data) {
		if(isCtrl) {
			switch(lastCtrl = DisplayCtrl.fromCode(data)) {
			case DISPLAYSOFF:
				screenOn[0] = false;
				screenOn[1] = false;
				break;
			case DISPLAY1ON:
				screenOn[0] = true;
				screenOn[1] = false;
				break;
			case DISPLAY2ON:
				screenOn[0] = false;
				screenOn[1] = true;
				break;
			default: /* pass */
			}
		} else {
			if(lastCtrl == null)
				throw new RuntimeException("Rouge data: " +
									data);
			switch(lastCtrl) {
			case DISPLAY:
				if(data != 0x10)
					throw new RuntimeException(
						"Invalid parameter " + data);
				break;
			case BRIGHT:
				// TODO z for now discard this
				// TODO ...
				break;
			case LOWERADDR1:  lowerScreenAddress [0] = data; break;
			case HIGHERADDR1: higherScreenAddress[0] = data; break;
			case LOWERADDR2:  lowerScreenAddress [1] = data; break;
			case HIGHERADDR2: higherScreenAddress[1] = data; break;
			case ADDRL:       lowerRWAddress         = data; break;
			case ADDRH:
				higherRWAddress = data;
				currentAddress = lowerRWAddress |
						(higherRWAddress << 8);
				break;
			case DATAWRITE:
				if(currentAddress >= memory.length)
					log.accept(String.format(
						"[WARNING ] DATAWRITE out of " +
						"bounds ignored with " +
						"address=0x%x, maxExcl=0x%x",
						currentAddress, memory.length));
				else
					memory[currentAddress++] = data;
				break;
			}
		}
	}

	public void accept16(boolean isCtrl, long data) {
		accept08(isCtrl, (data & 0xff00) >> 8);
		accept08(isCtrl, (data & 0x00ff));
	}

	public void accept32(boolean isCtrl, long data) {
		accept08(isCtrl, (data & 0xff000000) >> 24);
		accept08(isCtrl, (data & 0x00ff0000) >> 16);
		accept08(isCtrl, (data & 0x0000ff00) >> 8);
		accept08(isCtrl, (data & 0x000000ff));
	}

	boolean getScreenOn(int idx) {
		return screenOn[idx];
	}

	int getScreenAddressStart(int idx) {
		return lowerScreenAddress[idx] |
						(higherScreenAddress[idx] << 8);
	}

	int getMemory(int addr) {
		return memory[addr];
	}

}
