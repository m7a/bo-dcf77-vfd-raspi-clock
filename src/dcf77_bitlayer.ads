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

	Second_In_Us:            constant DCF77_Low_Level.Time := 1_000_000;
	Delay_Us_Target:         constant DCF77_Low_Level.Time :=   100_000;

	-- Wait 300ms for a signal to arrive before calling this a “no signal”
	-- case. 250ms is the longest signal accepted (limit for “1”),
	-- 21ms is 3x 7ms ISR execution intervals. The remainder (29ms) is extra
	-- safety. The idea is that large values here don't really hurt us
	-- because while the seconds will turn a little “late” they won't in
	-- general stay at 1.3s for long amounts of time, because while
	-- synchronized the clock stays with the signal and when out of sync
	-- the clock is 0.3s late but still turning at 1sec intervals in
	-- most cases.
	Timeout_No_Signal_In_Us: constant DCF77_Low_Level.Time :=   300_000;

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

	procedure Align_To_Slice(Ctx: in out Bitlayer;
					Time_Now: in out DCF77_Low_Level.Time);
	function Convert_Signal_Length_To_Reading(Ctx: in out Bitlayer;
					Signal_Length: in DCF77_Low_Level.Time)
					return Reading;
	function Detect_Second_Overflow(Ctx: in out Bitlayer;
					Time_Now: in DCF77_Low_Level.Time)
					return Reading;

end DCF77_Bitlayer;
