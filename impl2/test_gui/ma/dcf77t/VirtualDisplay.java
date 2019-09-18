package ma.dcf77t;

import javax.swing.JComponent;
import java.awt.Color;
import java.awt.Graphics;

class VirtualDisplay extends JComponent implements SerialDisplayInterface {

	private static enum Ctrl {

		DISPLAYSOFF(0x00),
		DISPLAY1ON(0x01),
		DISPLAY2ON(0x02),
		BRIGHT(0x13),
		DISPLAY(0x14),
/*

#define GP9002_DISPLAYSOFF        0x00
#define GP9002_DISPLAY1ON         0x01
#define GP9002_DISPLAY2ON         0x02
#define GP9002_ADDRINCR           0x04
#define GP9002_ADDRHELD           0x05
#define GP9002_CLEARSCREEN        0x06
#define GP9002_CONTROLPOWER       0x07
#define GP9002_DATAWRITE          0x08
#define GP9002_DATAREAD           0x09
#define GP9002_LOWERADDR1         0x0A
#define GP9002_HIGHERADDR1        0x0B
#define GP9002_LOWERADDR2         0x0C
#define GP9002_HIGHERADDR2        0x0D
#define GP9002_ADDRL              0x0E
#define GP9002_ADDRH              0x0F
#define GP9002_OR                 0x10
#define GP9002_XOR                0x11
#define GP9002_AND                0x12
#define GP9002_BRIGHT             0x13
#define GP9002_DISPLAY            0x14
#define GP9002_DISPLAY_MONOCHROME 0x10
#define GP9002_DISPLAY_GRAYSCALE  0x14
#define GP9002_INTMODE            0x15
#define GP9002_DRAWCHAR           0x20
#define GP9002_CHARRAM            0x21
#define GP9002_CHARSIZE           0x22
#define GP9002_CHARBRIGHT         0x24

*/

		NONE(-1);

		private final int code;

		private Ctrl(int code) {
			this.code = code;
		}

		static Ctrl fromCode(int code) {
			for(Ctrl c: Ctrl.class.getEnumConstants())
				if(c.code == code)
					return c;

			throw new RuntimeException("Unknown control code: " +
									code);
		}
	};

	private static final int DISPLAY_WIDTH_PX  = 128;
	private static final int DISPLAY_HEIGHT_PX =  64;
	private static final int PX_REPEAT = 3;

	private static final Color BG_ON  = new Color(0x50, 0x50, 0x50);
	private static final Color BG_OFF = Color.BLACK;
	private static final Color FG     = new Color(0x00, 0xff, 0xaa);

	/* y/x indexing. size similar to actual display */
	private int[][]   memory              = new int[8][];
	private boolean[] screenOn            = new boolean[] { false, false };
	private int[]     lowerScreenAddress  = new int[] { 0, 0 };
	private int[]     higherScreenAddress = new int[] { 0, 0 };

	/* protocol */
	private int       lowerRWAddress      = 0;
	private int       higherRWAddress     = 0;
	private Ctrl      lastCtrl            = Ctrl.NONE;

	VirtualDisplay() {
		super();
		for(int i = 0; i < memory.length; i++)
			memory[i] = new int[291];
	}

	@Override
	public void paintComponent(Graphics g) {
		g.setColor((screenOn[0] || screenOn[1])? BG_ON: BG_OFF);
		g.fillRect(0, 0, getWidth(), getHeight());
	}

	@Override
	public void write(boolean isCtrl, int data) {
		if(isCtrl) {
			switch(lastCtrl = Ctrl.fromCode(data)) {
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
				// TODO ...
				break;
			}
		}
	}

}
