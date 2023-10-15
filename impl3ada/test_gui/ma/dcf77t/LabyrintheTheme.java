// 2009 Ma_Sys.ma, originally from Labyrinthe 2
package ma.dcf77t;

import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.metal.DefaultMetalTheme;

public class LabyrintheTheme extends DefaultMetalTheme {

	private static final ColorUIResource[] p = {
		new ColorUIResource(66, 33, 66),
		new ColorUIResource(90, 86, 99),
		new ColorUIResource(99, 99, 99)
	};

	private static final ColorUIResource[] s = {
		new ColorUIResource(30,   30,  30),
		new ColorUIResource(50,   50,  50),
		new ColorUIResource(100, 100, 100)
	};
	
	static final ColorUIResource b = new ColorUIResource(200, 200, 200);
	static final ColorUIResource w = new ColorUIResource(60,   60,  60);
	
	public String getName() { return "Ma_Sys.ma Dark Swing Theme"; }
	
	protected ColorUIResource getPrimary1()   { return p[0]; }
	protected ColorUIResource getPrimary2()   { return p[1]; }
	protected ColorUIResource getPrimary3()   { return p[2]; }
	protected ColorUIResource getSecondary1() { return s[0]; }
	protected ColorUIResource getSecondary2() { return s[1]; }
	protected ColorUIResource getSecondary3() { return s[2]; }
	protected ColorUIResource getBlack()      { return b;    }
	protected ColorUIResource getWhite()      { return w;    }
}
