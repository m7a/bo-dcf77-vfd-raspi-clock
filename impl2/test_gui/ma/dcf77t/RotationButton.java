package ma.dcf77t;

import java.awt.Dimension;
import javax.swing.ButtonGroup;
import javax.swing.JPanel;
import javax.swing.JRadioButton;

class RotationButton {

	// TODO z currently these are out of thin air. should be practical
	//        measurement results.
	private static final int[] ASSIGNED_MEASUREMENTS = {
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60,
		0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0
	};

	private static final double ANGLE = Math.PI / 6;
	private static final double START_ANGLE = ANGLE * 6;
	private static final int OFFSET = 10;
	private static final double RADIUS = 80;

	private RotationButton() {}

	static JPanel create(final UserIOStatus uinput) {
		JPanel rv = new JPanel(null);

		ButtonGroup bg = new ButtonGroup();
		for(int i = 0; i < 12; i++) {
			int x = (int)Math.round(Math.cos(START_ANGLE +
					ANGLE * i) * RADIUS + RADIUS) + OFFSET;
			int y = (int)Math.round(Math.sin(START_ANGLE +
					ANGLE * i) * RADIUS + RADIUS) + OFFSET;
			JRadioButton btn = new JRadioButton(String.valueOf(
									i + 1));
			btn.setActionCommand(String.valueOf(
						ASSIGNED_MEASUREMENTS[i]));
			Dimension dim = btn.getPreferredSize();
			btn.setBounds(x, y, dim.width, dim.height);
			btn.addActionListener((ev) -> { uinput.wheel = Integer.
					parseInt(ev.getActionCommand()); });
			bg.add(btn);
			rv.add(btn);
			btn.setLocation(x, y);
		}

		rv.setBounds(0, 0, (int)Math.round(2 * RADIUS) + 2 * OFFSET,
				(int)Math.round(2 * RADIUS) + 2 * OFFSET);
		return rv;
	}

}
