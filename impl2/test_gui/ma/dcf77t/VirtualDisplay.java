package ma.dcf77t;

import javax.swing.JComponent;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;

import java.util.function.Consumer;

class VirtualDisplay extends JComponent {

	private static final int DISPLAY_WIDTH_PX    = 128;
	private static final int DISPLAY_HEIGHT_PX   =  64;
	private static final int SCREEN_HEIGHT_BYTES = DISPLAY_HEIGHT_PX / 8;
	private static final int SCREEN_NUM_BYTES    = DISPLAY_WIDTH_PX *
							SCREEN_HEIGHT_BYTES;
	private static final int PX_ENLARGE          =   3;
	private static final int PX_BETWEEN          =   1;

	private static final Color BG_ON  = new Color(0x30, 0x30, 0x30);
	private static final Color BG_OFF = Color.RED;
	private static final Color FG_ON  = new Color(0x00, 0xff, 0xee);
	private static final Color FG_OFF = new Color(0x90, 0x90, 0x90);

	private final VirtualDisplaySPI backend;

	VirtualDisplay(VirtualDisplaySPI backend) {
		super();
		this.backend = backend;
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
		g.setColor((backend.getScreenOn(0) || backend.getScreenOn(1))?
								BG_ON: BG_OFF);
		g.fillRect(0, 0, getWidth(), getHeight());

		if(backend.getScreenOn(0))
			drawScreen(g, 0);
		if(backend.getScreenOn(1))
			drawScreen(g, 1);
	}

	private void drawScreen(Graphics g, int index) {
		int screenAddressStart = backend.getScreenAddressStart(index);
		for(int i = 0; i < SCREEN_NUM_BYTES; i++) {
			int currentByte   = backend.getMemory(screenAddressStart
									+ i);
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

}
