package ma.dcf77t.telegram;

import javax.swing.JTextField;
import javax.swing.border.Border;
import java.awt.Font;

class ExactTextField extends JTextField {

	static final Font MTC_TERMINAL_FONT = new Font(Font.MONOSPACED,
								Font.PLAIN, 14);

	ExactTextField(String text, int width) {
		super(text, width);
		setFont(MTC_TERMINAL_FONT);
	}

	@Override public void setBorder(Border border) {
		// https://stackoverflow.com/questions/2281937
	}

}
