with DCF77_Low_Level;
use  DCF77_Low_Level;
with DCF77_Display;
use  DCF77_Display;

package body DCF77_Ambient_Light_Sensor is

	procedure Init(Ctx: in out ALS) is
	begin
		Ctx.Last_Idx := Brightness_Settings'Last;
	end Init;

	procedure Update(Ctx: in out ALS; Reading: in Light_Value;
						Setting: out Brightness) is
		Current_LV:  constant Light_Value :=
						Brightness_Limits(Ctx.Last_Idx);
		Proposed_LV: Light_Value;
		GT_Lim:      Natural := Brightness_Limits'First;
	begin
		for Idx in Brightness_Limits'Range loop
			if Reading >= Brightness_Limits(Idx) then
				GT_Lim := Idx;
			end if;
		end loop;
		Proposed_LV := Brightness_Limits(GT_Lim);
		if (Current_LV >= Required_Delta
			and then Proposed_LV < (Current_LV - Required_Delta))
		or else (Current_LV <= (Light_Value'Last - Required_Delta)
			and then Proposed_LV > (Current_LV + Required_Delta))
		then
			Ctx.Last_Idx := GT_Lim;
		end if;
		Setting := Brightness_Settings(Ctx.Last_Idx);
	end Update;

end DCF77_Ambient_Light_Sensor;
