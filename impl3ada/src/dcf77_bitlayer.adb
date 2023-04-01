with DCF77_Functions;

package body DCF77_Bitlayer is

	procedure Init(Ctx: in out Bitlayer; LL: access DCF77_Low_Level.LL) is
	begin
		Ctx.LL := LL;
	end Init;

	function Update(Ctx: in out Bitlayer) return Reading is
		use type DCF77_Low_Level.Time;
		Signal_Length, Signal_Begin: DCF77_Low_Level.Time;
		R: Reading;
		Aligned: Boolean;
	begin
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
			if not Aligned then
				Ctx.LL.Delay_Micros(25_000);
			end if;
		else
			Ctx.Update_No_Signal(R);
		end if;
		return R;
	end Update;

	procedure Update_Signal(Ctx: in out Bitlayer; 
			Signal_Length_Ms, Signal_Start_Ago_Ms: in Natural;
			R: out Reading; Aligned: out Boolean) is
	begin
		Aligned := (Signal_Start_Ago_Ms > 20);
		Ctx.Intervals_Of_100ms_Passed := 0;

		if Signal_Length_Ms in 20 .. 150 then
			R := Bit_0;
		elsif Signal_Length_Ms in 151 .. 450 then
			R := Bit_1;
		else
			R := No_Signal;
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

end DCF77_Bitlayer;
