with DCF77_Low_Level;
with DCF77_Display;

package DCF77_Ambient_Light_Sensor is

	type ALS is tagged limited private;

	procedure Init(Ctx: in out ALS);
	procedure Update(Ctx: in out ALS;
				Reading: in DCF77_Low_Level.Light_Value;
				Setting: out DCF77_Display.Brightness);

private

	Brightness_Settings: constant array (1 .. 8) of
						DCF77_Display.Brightness := (
		DCF77_Display.Display_Brightness_Perc_030,
		DCF77_Display.Display_Brightness_Perc_040,
		DCF77_Display.Display_Brightness_Perc_050,
		DCF77_Display.Display_Brightness_Perc_060,
		DCF77_Display.Display_Brightness_Perc_070,
		DCF77_Display.Display_Brightness_Perc_080,
		DCF77_Display.Display_Brightness_Perc_090,
		DCF77_Display.Display_Brightness_Perc_100
	);
	Brightness_Limits: constant array (1 .. 8) of
						DCF77_Low_Level.Light_Value := (
		 9,
		18,
		27,
		36,
		45,
		54,
		63,
		72
	);
	Required_Delta: constant DCF77_Low_Level.Light_Value := 7;

	type ALS is tagged limited record
		Last_Idx: Natural;
	end record;

end DCF77_Ambient_Light_Sensor;
