package ma.dcf77t;

class TestVirtualDisplay {

	private static final int[][] INITIALIZATION_SEQUENCE = {
		/* mode / initialization */
		{ 1, DisplayCtrl.DISPLAY.code },     { 0, 0x10 },
		/* brightness */
		{ 1, DisplayCtrl.BRIGHT.code },      { 0, 0x00 },
		/* 2nd screen starts at 0x400 */
		{ 1, DisplayCtrl.LOWERADDR2.code  }, { 0, 0x00 },
		{ 1, DisplayCtrl.HIGHERADDR2.code }, { 0, 0x04 },
		/* clear and show vscreen 0 */
		{ 1, DisplayCtrl.CLEARSCREEN.code },
		{ 1, DisplayCtrl.DISPLAY1ON.code },
	};

	private TestVirtualDisplay() {}

	static void testVirtualDisplay(VirtualDisplaySPI d) {
		for(int[] cmd: INITIALIZATION_SEQUENCE)
			d.accept(cmd[0] == 1, cmd[1]);

		// For testing, we would like to display a rectangle of size
		// 40x20 at 5,10. The address for 5,10 is 0x20 (X) + 1 (Y)
		// which indicates point 5,8 -> set first two most significant
		// bits to 0 for the first part. To draw of size 20 we need
		// to set 6 bits from the first byte, then 8 bits in the next
		// and finally the uppermost 6 bits in the next giving values
		// 0x3f,0xff,0xfc from start address 0x21 for the first
		// segment of 40. The next segments are obtained by computing
		// the start address + 8bytes 

		int[] data = { 0x3f, 0xff, 0xfc };
		for(int address = 0x21, numW = 0; numW < 40;
							address += 8, numW++) {
			int lower  = address & 0xff;
			int higher = (address & 0xf00) >> 8;
			d.accept(true, DisplayCtrl.ADDRL.code);
			d.accept(false, lower);
			d.accept(true, DisplayCtrl.ADDRH.code);
			d.accept(false, higher);
			d.accept(true, DisplayCtrl.DATAWRITE.code);
			for(int b: data)
				d.accept(false, b);
		}
	}

}
