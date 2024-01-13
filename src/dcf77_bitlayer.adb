with DCF77_Functions;
with DCF77_Low_Level;
use  DCF77_Low_Level;

package body DCF77_Bitlayer is

	procedure Init(Ctx: in out Bitlayer; LL: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL             := LL;
		Ctx.Start_Of_Sec   := LL.Get_Time_Micros;
		Ctx.Start_Of_Slice := Ctx.Start_Of_Sec;
		Ctx.Delay_Us       := Delay_Us_Target;
		Ctx.Unidentified   := 0;
		Ctx.Overflown      := 0;
	end Init;

	function Update_TIck(Ctx: in out Bitlayer) return Reading is
		End_Of_Sec:   constant Time := Ctx.Start_Of_Sec + Second_In_Us;
		End_Of_Slice: constant Time := Ctx.Start_Of_Slice +
								Delay_Us_Target;
		Time_Now:      Time := Ctx.LL.Get_Time_Micros;
		Signal_Length: Time;
		Signal_Begin:  Time;
		R:             Reading;
	begin
		if Time_Now > End_Of_Slice then
			DCF77_Functions.Inc_Saturated(Ctx.Overflown,
							Bitlayer_Fault_Max);
		else
			Ctx.Delay_Us := End_Of_Slice - Time_Now;
			if Time_Now < End_Of_Slice then
				Ctx.LL.Delay_Until(End_Of_Slice);
			end if;
			Time_Now := Ctx.LL.Get_Time_Micros;
		end if;
		Ctx.Start_Of_Slice := Time_Now;

		if Ctx.LL.Read_Interrupt_Signal(Signal_Length, Signal_Begin)
									then
			if Signal_Length in 50_000 .. 140_000 then
				R := Bit_0;
			elsif Signal_Length in 150_000 .. 250_000 then
				R := Bit_1;
			else
				R := No_Signal;
				Ctx.LL.Log("Unidentified: " & Time'Image(
							Signal_Length));
				DCF77_Functions.Inc_Saturated(Ctx.Unidentified,
							Bitlayer_Fault_Max);
			end if;
			Ctx.Start_Of_Sec := Ctx.Start_Of_Sec + Second_In_Us;
		else
			if Time_Now > (End_Of_Sec + Delay_Us_Epsilon) then
				R := No_Signal;
				Ctx.Start_Of_Sec := Ctx.Start_Of_Sec +
							Second_In_Us;
			else
				R := No_Update;
			end if;
		end if;
		return R;
	end Update_Tick;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural is
							(Ctx.Unidentified);
	function Get_Overflown(Ctx: in Bitlayer) return Natural is
							(Ctx.Overflown);
	function Get_Delay(Ctx: in Bitlayer) return Time is (Ctx.Delay_Us);

end DCF77_Bitlayer;
