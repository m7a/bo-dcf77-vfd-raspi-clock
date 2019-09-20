package ma.dcf77t;

import java.awt.Dimension;
import javax.swing.ButtonGroup;
import javax.swing.JPanel;
import javax.swing.JRadioButton;

class RotationButton {

	private static final double ANGLE = Math.PI / 6;
	private static final double START_ANGLE = ANGLE * 6;
	private static final int OFFSET = 10;
	private static final double RADIUS = 80;

	private RotationButton() {}

	static JPanel create() {
		JPanel rv = new JPanel(null);
		//double maxX = 0; 
		//double maxY = 0;

		ButtonGroup bg = new ButtonGroup();

		for(int i = 0; i < 12; i++) {
			int x = (int)Math.round(Math.cos(START_ANGLE +
					ANGLE * i) * RADIUS + RADIUS) + OFFSET;
			int y = (int)Math.round(Math.sin(START_ANGLE +
					ANGLE * i) * RADIUS + RADIUS) + OFFSET;
			JRadioButton btn = new JRadioButton(String.valueOf(
									i + 1));
			Dimension dim = btn.getPreferredSize();
			btn.setBounds(x, y, dim.width, dim.height);
			bg.add(btn);
			rv.add(btn);
			btn.setLocation(x, y);
		}

		rv.setBounds(0, 0, (int)Math.round(2 * RADIUS) + 2 * OFFSET,
				(int)Math.round(2 * RADIUS) + 2 * OFFSET);
		return rv;
	}

}
