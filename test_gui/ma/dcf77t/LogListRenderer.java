package ma.dcf77t;

import java.awt.Color;
import java.awt.Component;
import javax.swing.JList;
import javax.swing.DefaultListCellRenderer;

class LogListRenderer extends DefaultListCellRenderer {

	private static final Color LIGHT_BLUE = new Color(0x90, 0xe0, 0xff);
	private static final Color LESS_LIGHT_BLUE =
						new Color(0x50, 0x70, 0xff);

	@Override
	public Component getListCellRendererComponent(JList list, Object value,
			int index, boolean isSelected, boolean cellHasFocus) {
		Component c = super.getListCellRendererComponent(list, value,
					index, isSelected, cellHasFocus);

		if(value != null) {
			String str = value.toString();

			if(str.startsWith("[INFO    ]"))
				c.setForeground(Color.GREEN);
			if (str.startsWith("[ADALOG  ]"))
				c.setForeground(Color.CYAN);
			else if(str.startsWith("[WARNING ]"))
				c.setForeground(Color.YELLOW);
			else if(str.startsWith("[ERROR   ]"))
				c.setForeground(Color.RED);
			else if(str.matches("^\\[........\\] > .*$"))
				c.setForeground(LIGHT_BLUE);
			else if(str.matches("^\\[........\\] .*$"))
				c.setForeground(LESS_LIGHT_BLUE);
			else
				c.setForeground(Color.WHITE);
		}

		return c;
	}

}
