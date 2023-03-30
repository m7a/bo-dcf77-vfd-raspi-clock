with DCF77_Low_Level;
with DCF77_Types;
use  DCF77_Types;

package DCF77_Bitlayer is

	type Bitlayer is tagged limited private;

	procedure Init(Ctx: in out Bitlayer; LL: access DCF77_Low_Level.LL);
	function Update(Ctx: in out Bitlayer) return Reading;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural;

private

	Bitlayer_Fault_Max: constant Natural := 1000;

	type Bitlayer is tagged limited record
		LL:                        access DCF77_Low_Level.LL;
		Intervals_Of_100ms_Passed: Natural := 0;
		Unidentified:              Natural := 0;
	end record;

	procedure Update_Signal(Ctx: in out Bitlayer; 
			Signal_Length_Ms, Signal_Start_Ago_Ms: in Natural;
			R: out Reading; Aligned: out Boolean);
	procedure Update_No_Signal(Ctx: in out Bitlayer; R: out Reading);


end DCF77_Bitlayer;
