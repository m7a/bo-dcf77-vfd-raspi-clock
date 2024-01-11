with DCF77_Low_Level;
use  DCF77_Low_Level;
with DCF77_Display;
use  DCF77_Display;

package body DCF77_Ambient_Light_Sensor is

	procedure Init(Ctx: in out ALS) is
	begin
		Ctx.Last_Brightness := Display_Brightness_Perc_100;
	end Init;

	procedure Update(Ctx: in out ALS; Reading: in Light_Value;
						Setting: out Brightness) is
	begin
		-- TODO PUT REAL ALGORITHM HERE
		if Reading <= 30 then
			Ctx.Last_Brightness := Display_Brightness_Perc_040;
		else
			Ctx.Last_Brightness := Display_Brightness_Perc_060;
		end if;
		Setting := Ctx.Last_Brightness;
	end Update;

end DCF77_Ambient_Light_Sensor;
