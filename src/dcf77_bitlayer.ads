with DCF77_Low_Level;
with DCF77_Types;
use  DCF77_Types;

package DCF77_Bitlayer is

	type Bitlayer is tagged limited private;

	procedure Init(Ctx: in out Bitlayer; LL: in DCF77_Low_Level.LLP);
	function Update_Tick(Ctx: in out Bitlayer) return Reading;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural;
	function Get_Delay(Ctx: in out Bitlayer) return DCF77_Low_Level.Time;

private

	Delay_Us_Target:    constant DCF77_Low_Level.Time := 100_000;
	Bitlayer_Fault_Max: constant Natural              := 1000;

	type Bitlayer is tagged limited record
		LL:                        DCF77_Low_Level.LLP;
		Intervals_Of_100ms_Passed: Natural := 0;
		Unidentified:              Natural := 0;
		Delay_Us:     DCF77_Low_Level.Time := Delay_Us_Target;
		Time_Old:     DCF77_Low_Level.Time;
	end record;

	procedure Tick(Ctx: in out Bitlayer);
	procedure Update_Signal(Ctx: in out Bitlayer; 
			Signal_Length_Ms, Signal_Start_Ago_Ms: in Natural;
			R: out Reading; Aligned: out Boolean);
	procedure Update_No_Signal(Ctx: in out Bitlayer; R: out Reading);

end DCF77_Bitlayer;
