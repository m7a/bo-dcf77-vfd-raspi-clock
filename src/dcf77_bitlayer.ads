with DCF77_Low_Level;
with DCF77_Types;
use  DCF77_Types;

package DCF77_Bitlayer is

	type Bitlayer is tagged limited private;

	procedure Init(Ctx: in out Bitlayer; LL: in DCF77_Low_Level.LLP);
	function Update_Tick(Ctx: in out Bitlayer) return Reading;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural;
	function Get_Discarded(Ctx: in Bitlayer) return Natural;
	function Get_Overflown(Ctx: in Bitlayer) return Natural;
	function Get_Delay(Ctx: in Bitlayer) return DCF77_Low_Level.Time;

private

	Second_In_Us:       constant DCF77_Low_Level.Time := 1_000_000;
	Delay_Us_Target:    constant DCF77_Low_Level.Time :=   100_000;
	Delay_Us_Epsilon:   constant DCF77_Low_Level.Time :=    20_000;

	type Bitlayer is tagged limited record
		-- required
		LL:               DCF77_Low_Level.LLP;
		Start_Of_Sec:     DCF77_Low_Level.Time;
		Start_Of_Slice:   DCF77_Low_Level.Time;
		Preceding_Signal: DCF77_Low_Level.Time;
		-- informational
		Delay_Us:         DCF77_Low_Level.Time;
		Unidentified:     Natural;
		Discarded:        Natural;
		Overflown:        Natural;
	end record;

end DCF77_Bitlayer;
