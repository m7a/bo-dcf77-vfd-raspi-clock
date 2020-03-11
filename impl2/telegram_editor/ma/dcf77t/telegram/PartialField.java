package ma.dcf77t.telegram;

import javax.swing.*;
import java.awt.Font;
import java.awt.Color;
import java.awt.GridLayout;

class PartialField extends JPanel {

	final int width;

	private final JLabel descr;
	private final JTextField endec; // encode decode field

	PartialField(String text, int width) {
		super(new GridLayout(2, 1));
		this.width = width;
		add(descr = new JLabel(text));
		add(endec = new ExactTextField(text, width));
		descr.setFont(ExactTextField.MTC_TERMINAL_FONT);
	}

	@Override public void setBackground(Color c) {
		super.setBackground(c);
		if(endec != null)
			endec.setBackground(c);
	}

	@Override public void setEnabled(boolean enabled) {
		super.setEnabled(enabled);
		descr.setEnabled(enabled);
		endec.setEnabled(enabled);
	}

	void setText(String text) {
		endec.setText(text);
	}

	String getText() {
		return endec.getText();
	}

}
