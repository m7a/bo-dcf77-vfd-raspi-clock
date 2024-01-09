with DCF77_Functions;
with DCF77_Low_Level;
use  DCF77_Low_Level;

-- TODO HEAVY DRIFT WHEN RUN W/O SYNC! NEED TO ADJUST THIS SOMEHOW...
-- In principle we have to adjust the bitlayer to query
-- one earlier when the preceding output was No_Signal.
-- Since that was after 11 iterations, next check after
-- 9 iterations to sum up to 20 i.e. 2000ms or something.

package body DCF77_Bitlayer is

	procedure Init(Ctx: in out Bitlayer; LL: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL       := LL;
		Ctx.Time_Old := LL.Get_Time_Micros;
	end Init;

	function Update_Tick(Ctx: in out Bitlayer) return Reading is
		use type DCF77_Low_Level.Time;
		Signal_Length, Signal_Begin: DCF77_Low_Level.Time;
		R: Reading;
		Aligned: Boolean;
	begin
		Ctx.Tick;
		if Ctx.LL.Read_Interrupt_Signal(Signal_Length,
							Signal_Begin) then
			Ctx.Update_Signal(
				Natural(Signal_Length / 1_000),
				Natural((Ctx.LL.Get_Time_Micros - Signal_Begin)
								/ 1_000),
				R, Aligned
			);
			-- Next time query earlier by making this
			-- execution take longer!
			-- TODO CURRENTLY INCORRECT BECAUSE TICKER NO LONGER ALLOWS THIS TO WORK!
			if not Aligned then
				Ctx.LL.Log("MISALIGNDLY");
				Ctx.LL.Delay_Micros(10_000);
			end if;
		else
			Ctx.Update_No_Signal(R);
		end if;
		return R;
	end Update_Tick;

	procedure Tick(Ctx: in out Bitlayer) is
		Time_Target: constant Time := Ctx.Time_Old + Delay_Us_Target;
		Time_New:             Time := Ctx.LL.Get_Time_Micros;
	begin
		-- TODO no overflow protection whatsoever.
		Ctx.Delay_Us := Delay_Us_Target - (Time_New - Ctx.Time_Old);
		while Time_New < Time_Target loop
			Time_New := Ctx.LL.Get_Time_Micros;
		end loop;
		Ctx.Time_Old := Time_New;
	end Tick;

	procedure Update_Signal(Ctx: in out Bitlayer; 
			Signal_Length_Ms, Signal_Start_Ago_Ms: in Natural;
			R: out Reading; Aligned: out Boolean) is
	begin
		Aligned := (Signal_Start_Ago_Ms > 20);
		Ctx.Intervals_Of_100ms_Passed := 0;

		if Signal_Length_Ms in 50 .. 140 then
			R := Bit_0;
		elsif Signal_Length_Ms in 150 .. 250 then
			R := Bit_1;
		else
			R := No_Signal;
			Ctx.LL.Log("UNIDENTIFIED: " &
					Natural'Image(Signal_Length_Ms));
			DCF77_Functions.Inc_Saturated(Ctx.Unidentified,
							Bitlayer_Fault_Max);
		end if;
	end Update_Signal;

	procedure Update_No_Signal(Ctx: in out Bitlayer; R: out Reading) is
	begin
		Ctx.Intervals_Of_100ms_Passed :=
					Ctx.Intervals_Of_100ms_Passed + 1;

		if Ctx.Intervals_Of_100ms_Passed >= 11 then
			Ctx.Intervals_Of_100ms_Passed := 0;
			R := No_Signal;
		else
			R := No_Update;
		end if;
	end Update_No_Signal;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural is
							(Ctx.Unidentified);

	function Get_Delay(Ctx: in out Bitlayer) return Time is (Ctx.Delay_Us);

end DCF77_Bitlayer;
