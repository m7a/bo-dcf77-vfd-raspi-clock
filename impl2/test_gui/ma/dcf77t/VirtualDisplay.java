package ma.dcf77t;

import javax.swing.JComponent;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;

class VirtualDisplay extends JComponent implements SerialDisplayInterface {

	private static final int DISPLAY_WIDTH_PX    = 128;
	private static final int DISPLAY_HEIGHT_PX   =  64;
	private static final int SCREEN_HEIGHT_BYTES = DISPLAY_HEIGHT_PX / 8;
	private static final int SCREEN_NUM_BYTES    = DISPLAY_WIDTH_PX *
							SCREEN_HEIGHT_BYTES;
	private static final int PX_ENLARGE          =   3;
	private static final int PX_BETWEEN          =   1;

	private static final Color BG_ON  = new Color(0x30, 0x30, 0x30);
	private static final Color BG_OFF = Color.BLACK;
	private static final Color FG_ON  = new Color(0x00, 0xff, 0xee);
	private static final Color FG_OFF = new Color(0x90, 0x90, 0x90);

	private int[]       memory              = new int[0x909];
	private boolean[]   screenOn            = new boolean[] { false, false };
	private int[]       lowerScreenAddress  = new int[] { 0, 0 };
	private int[]       higherScreenAddress = new int[] { 0, 0 };

	/* protocol */
	private int         lowerRWAddress      = 0;
	private int         higherRWAddress     = 0;
	private int         currentAddress      = 0; /* auto increment */
	private DisplayCtrl lastCtrl            = DisplayCtrl.NONE;

	VirtualDisplay() {
		super();
	}

	@Override
	public Dimension getPreferredSize() {
		return new Dimension(
			DISPLAY_WIDTH_PX  * (PX_ENLARGE + PX_BETWEEN),
			DISPLAY_HEIGHT_PX * (PX_ENLARGE + PX_BETWEEN)
		);
	}

	@Override
	public void paintComponent(Graphics g) {
		g.setColor((screenOn[0] || screenOn[1])? BG_ON: BG_OFF);
		g.fillRect(0, 0, getWidth(), getHeight());

		if(screenOn[0])
			drawScreen(g, 0);
		if(screenOn[1])
			drawScreen(g, 1);
	}

	private void drawScreen(Graphics g, int index) {
		int screenAddressStart = lowerScreenAddress[index] |
					(higherScreenAddress[index] << 8);
		for(int i = 0; i < SCREEN_NUM_BYTES; i++) {
			int currentByte   = memory[screenAddressStart + i];
			int virtualStartY = (i % SCREEN_HEIGHT_BYTES) * 8;
			int virtualStartX = i / SCREEN_HEIGHT_BYTES;

			for(int j = 0; j < 8; j++) {
				// 7 - j because we start from the msb.
				boolean isPixelOn =
					(((currentByte >> (7 - j)) & 1) == 1);
				g.setColor(isPixelOn? FG_ON: FG_OFF);
				int realY = (virtualStartY + j) *
						(PX_ENLARGE + PX_BETWEEN);
				int realX = virtualStartX *
						(PX_ENLARGE + PX_BETWEEN);
				g.fillRect(realX, realY, PX_ENLARGE,
								PX_ENLARGE);
			}
		}
	}

	@Override
	public void write(boolean isCtrl, int data) {
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
			case DATAWRITE: memory[currentAddress++] = data; break;
			}
		}
	}

}
