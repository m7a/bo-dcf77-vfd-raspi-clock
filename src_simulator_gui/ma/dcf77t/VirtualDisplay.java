package ma.dcf77t;

import javax.swing.JComponent;
import javax.swing.JFileChooser;
import javax.swing.filechooser.FileNameExtensionFilter;
import javax.imageio.ImageIO;
import java.awt.event.ActionEvent;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.awt.image.DataBuffer;
import java.util.Arrays;
import java.util.function.Consumer;
import java.io.IOException;
import java.io.File;

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
		int[] memory = backend.getMemory();
		for(int i = 0; i < SCREEN_NUM_BYTES; i++) {
			int virtualStartY = (i % SCREEN_HEIGHT_BYTES) * 8;
			int virtualStartX = i / SCREEN_HEIGHT_BYTES;
			int currentByte = memory[screenAddressStart + i];

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

	void screenshot(ActionEvent ev) {
		boolean isEmpty;
		BufferedImage img = new BufferedImage(
					DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX,
					BufferedImage.TYPE_BYTE_BINARY);
		do {
			int[] memory    = backend.getMemory();
			int[] memoryCPY = Arrays.copyOf(memory, memory.length);
			int screenID    = backend.getScreenOn(0) ? 0 :
					(backend.getScreenOn(1) ? 1 : -1);
			if (screenID == -1)
				return;

			int screenAddressStart = backend.getScreenAddressStart(
								screenID);
			isEmpty = true;
			for (int i = 0; i < SCREEN_NUM_BYTES; i++) {
				int virtualStartY =
						(i % SCREEN_HEIGHT_BYTES) * 8;
				int virtualStartX = i / SCREEN_HEIGHT_BYTES;
				int currentByte = memoryCPY[i];
				for (int j = 0; j < 8; j++) {
					boolean isPixelOn = (((currentByte >>
							(7 - j)) & 1) == 1);
					if (isPixelOn)
						isEmpty = false;
					img.setRGB(virtualStartX,
							virtualStartY + j,
							isPixelOn ?  0x000000 :
								0xffffff);
				}
			}
			if (isEmpty) {
				try {
					Thread.sleep(30);
				} catch(InterruptedException ex) {
					// ignore
				}
			}
		} while (isEmpty);

		JFileChooser chooser = new JFileChooser();
		chooser.setFileFilter(new FileNameExtensionFilter("PNG files",
									"png"));
		File file;

		switch (chooser.showSaveDialog(this)) {
		case JFileChooser.APPROVE_OPTION:
			file = chooser.getSelectedFile();
			break;
		default:
			return;
		}

		try {
			ImageIO.write(img, "png", file);
		} catch(IOException ex) {
			ex.printStackTrace();
		}
	}

}
