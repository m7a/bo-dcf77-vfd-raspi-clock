package ma.dcf77t;

import java.awt.*;
import javax.swing.*;

class AppWnd {

	private final JFrame wnd;

	public AppWnd() {
		wnd = new JFrame("Ma_Sys.ma DCF-77 VFD Module Clock " +
						"Interactive Test Application");
		Container cp = wnd.getContentPane();
		cp.setLayout(new BorderLayout());

		Box virtualClock = new Box(BoxLayout.Y_AXIS);
		cp.add(BorderLayout.EAST, virtualClock);

		Box analysisUnit = new Box(BoxLayout.Y_AXIS);
		cp.add(BorderLayout.CENTER, analysisUnit);

		wnd.setSize(800, 600);
		wnd.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}

	public void show() {
		wnd.setVisible(true);
	}

}
