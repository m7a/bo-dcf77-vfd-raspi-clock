package ma.dcf77t.telegram;

import java.awt.Toolkit;
import javax.swing.UIManager;
import javax.swing.plaf.metal.MetalLookAndFeel;
import javax.swing.plaf.metal.MetalTheme;

public class Main {

	public static void main(String[] args) {
		loadTheme();
		new AppWnd();
	}

	private static void loadTheme() {
		System.setProperty("sun.awt.noerasebackground", "true");
		Toolkit.getDefaultToolkit().setDynamicLayout(true);
		MetalTheme theme = new LabyrintheTheme();
		MetalLookAndFeel.setCurrentTheme(theme);
		try {
			UIManager.setLookAndFeel(
				"javax.swing.plaf.metal.MetalLookAndFeel");
		} catch(Exception ex) {
			ex.printStackTrace();
		}
	}

}
