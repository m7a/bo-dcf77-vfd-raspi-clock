with DCF77_Functions;
with DCF77_Low_Level;
use  DCF77_Low_Level;

package body DCF77_Bitlayer is

	procedure Init(Ctx: in out Bitlayer; LL: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL               := LL;
		Ctx.Start_Of_Sec     := LL.Get_Time_Micros;
		Ctx.Start_Of_Slice   := Ctx.Start_Of_Sec;
		Ctx.Preceding_Signal := 0;
		Ctx.Delay_Us         := Delay_Us_Target;
		Ctx.Unidentified     := 0;
		Ctx.Discarded        := 0;
		Ctx.Overflown        := 0;
	end Init;

	function Update_TIck(Ctx: in out Bitlayer) return Reading is
		Time_Now: Time := Ctx.LL.Get_Time_Micros;
		Signal_Length: Time;
		Signal_Begin: Time;
		R: Reading;
	begin
		Ctx.Align_To_Slice(Time_Now);

		-- When within close sequence, multiple “valid” bits are
		-- observed, the subsequent ones are discarded as to not make
		-- the clock run faster than intended.
		if Ctx.LL.Read_Interrupt_Signal(Signal_Length, Signal_Begin)
									then
			R := Ctx.Convert_Signal_Length_To_Reading(
								Signal_Length);
			if R /= No_Update then
				if Signal_Begin - Ctx.Preceding_Signal
								< 750_000 then
					-- We have to discard the reading if we
					-- already had one within the preceding
					-- 750 (800) ms.
					Ctx.LL.Log("Discard: " &
						Time'Image(Signal_Length));
					DCF77_Functions.Inc_Saturated(
							Ctx.Discarded, 9999);
				else
					-- If it is all good, record the signal
					-- begin as valid time to compare
					-- subsequent signals against
					Ctx.Preceding_Signal := Signal_Begin;
					Ctx.Start_Of_Sec     := Signal_Begin;
					-- no overflow condition in event of
					-- valid signal!
					return R;
				end if;
			end if;
		end if;

		-- catch all / all “else” cases except for the good case go here
		return Ctx.Detect_Second_Overflow(Time_Now);
	end Update_Tick;

	-- Slices are the tick interval at which the display is updated and the
	-- interrupt readings are queried. Here, it is aligned to be every 100ms
	-- automatically adjusting to the computation time that the preceding
	-- code took.
	procedure Align_To_Slice(Ctx: in out Bitlayer; Time_Now: in out Time) is
		End_Of_Slice: constant Time := Ctx.Start_Of_Slice +
								Delay_Us_Target;
	begin
		if Time_Now > End_Of_Slice then
			DCF77_Functions.Inc_Saturated(Ctx.Overflown, 99);
		else
			Ctx.Delay_Us := End_Of_Slice - Time_Now;
			if Time_Now < End_Of_Slice then
				Ctx.LL.Delay_Until(End_Of_Slice);
			end if;
			Time_Now := Ctx.LL.Get_Time_Micros;
		end if;
		Ctx.Start_Of_Slice := Time_Now;
	end Align_To_Slice;

	-- 50..140ms  -> zero bit
	-- 150..250ms -> one bit
	-- others     -> invalid reading
	function Convert_Signal_Length_To_Reading(Ctx: in out Bitlayer;
				Signal_Length: in Time) return Reading is
	begin
		-- These signal lengths in general identify a valid reading
		if Signal_Length in 50_000 .. 140_000 then
			return Bit_0;
		elsif Signal_Length in 150_000 .. 250_000 then
			return Bit_1;
		else
			Ctx.LL.Log("Unidentified: " &
						Time'Image(Signal_Length));
			DCF77_Functions.Inc_Saturated(Ctx.Unidentified, 9999);
			return No_Update;
		end if;
	end Convert_Signal_Length_To_Reading;

	-- If no valid signal was produced (any of no interrupt or wrong signal
	-- length or discarded signal) then check if we have exceeded 1sec and
	-- switch the second counter despite not having received anything
	-- valuable. Otherwise say “No Update”.
	function Detect_Second_Overflow(Ctx: in out Bitlayer;
					Time_Now: in Time) return Reading is
		End_Of_Sec: constant Time := Ctx.Start_Of_Sec + Second_In_Us;
	begin
		if Time_Now > End_Of_Sec + Timeout_No_Signal_In_Us then
			Ctx.Start_Of_Sec := End_Of_Sec;
			return No_Signal;
		else
			return No_Update;
		end if;
	end Detect_Second_Overflow;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural is
							(Ctx.Unidentified);
	function Get_Discarded(Ctx: in Bitlayer) return Natural is
							(Ctx.Discarded);
	function Get_Overflown(Ctx: in Bitlayer) return Natural is
							(Ctx.Overflown);
	function Get_Delay(Ctx: in Bitlayer) return Time is (Ctx.Delay_Us);

end DCF77_Bitlayer;
