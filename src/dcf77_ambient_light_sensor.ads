with DCF77_Low_Level;
with DCF77_Display;

package DCF77_Ambient_Light_Sensor is

	type ALS is tagged limited private;

	procedure Init(Ctx: in out ALS);
	procedure Update(Ctx: in out ALS;
				Reading: in DCF77_Low_Level.Light_Value;
				Setting: out DCF77_Display.Brightness);

private

	type ALS is tagged limited record
		Last_Brightness: DCF77_Display.Brightness;
	end record;

end DCF77_Ambient_Light_Sensor;
