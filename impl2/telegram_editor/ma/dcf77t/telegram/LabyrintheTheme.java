/*
 *    This file is part of Labyrinthe.
 *
 *    Labyrinthe is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Labyrinthe is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Labyrinthe.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    Copyright (C) 2009 Ma_Sys.ma
 */
package ma.dcf77t.telegram;

import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.metal.DefaultMetalTheme;

public class LabyrintheTheme extends DefaultMetalTheme {
	
    public String getName() { return "Labyrinthe Game Theme"; }
	
    private final ColorUIResource primary1 = new ColorUIResource(66, 33, 66);
    private final ColorUIResource primary2 = new ColorUIResource(90, 86, 99);
    private final ColorUIResource primary3 = new ColorUIResource(99, 99, 99);

    private final ColorUIResource secondary1 = new ColorUIResource(30, 30, 30);
    private final ColorUIResource secondary2 = new ColorUIResource(50, 50, 50);
    private final ColorUIResource secondary3 = new ColorUIResource(100, 100, 100);

    private final ColorUIResource black = new ColorUIResource(200, 200, 200);
    private final ColorUIResource white = new ColorUIResource(60, 60, 60);

    protected ColorUIResource getPrimary1() { return primary1; }
    protected ColorUIResource getPrimary2() { return primary2; }
    protected ColorUIResource getPrimary3() { return primary3; }

    protected ColorUIResource getSecondary1() { return secondary1; }
    protected ColorUIResource getSecondary2() { return secondary2; }
    protected ColorUIResource getSecondary3() { return secondary3; }

    protected ColorUIResource getBlack() { return black; }
    protected ColorUIResource getWhite() { return white; }

}
