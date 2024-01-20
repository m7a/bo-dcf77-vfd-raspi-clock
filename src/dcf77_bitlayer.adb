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
		End_Of_Sec:   constant Time := Ctx.Start_Of_Sec + Second_In_Us;
		End_Of_Slice: constant Time := Ctx.Start_Of_Slice +
								Delay_Us_Target;
		Has_Reading:   Boolean := False;
		Time_Now:      Time := Ctx.LL.Get_Time_Micros;
		Signal_Length: Time;
		Signal_Begin:  Time;
		R:             Reading;
	begin
		-- Slices are the tick interval at which the display is updated
		-- and the interrupt readings are queried. Here, it is aligned
		-- to be every 100ms automatically adjusting to the computation
		-- time that the preceding code took.
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

		-- Read the interrupt signal
		--
		-- 50..140ms  -> zero bit
		-- 150..250ms -> one bit
		-- others     -> invalid reading
		--
		-- When within close sequence, multiple “valid” bits are
		-- observed, the subsequent ones are discarded as to not make
		-- the clock run faster than intended.
		if Ctx.LL.Read_Interrupt_Signal(Signal_Length, Signal_Begin)
									then
			-- These signal lengths in general identify a valid
			-- reading
			if Signal_Length in 50_000 .. 140_000 then
				Has_Reading := True;
				R           := Bit_0;
			elsif Signal_Length in 150_000 .. 250_000 then
				Has_Reading := True;
				R           := Bit_1;
			else
				R := No_Update;
				Ctx.LL.Log("Unidentified: " & Time'Image(
							Signal_Length));
				DCF77_Functions.Inc_Saturated(Ctx.Unidentified,
									9999);
			end if;
			-- We have to discard the reading if we already had
			-- one within the preceding 750 (800) ms.
			if Has_Reading and (Signal_Begin -
					Ctx.Preceding_Signal) < 750_000 then
				Has_Reading := False;
				R           := No_Update;
				Ctx.LL.Log("Discard: " &
						Time'Image(Signal_Length));
				DCF77_Functions.Inc_Saturated(Ctx.Discarded,
									9999);
			else
			-- If it is all good, record the signal begin as valid
			-- time to compare subsequent signals against
				Ctx.Preceding_Signal := Signal_Begin;
			end if;
		end if;
		-- If no valid signal was produced (any of no interrupt or
		-- wrong signal length or discarded signal) then check if we
		-- have exceeded 1sec and switch the second counter despite not
		-- having received anything valuable. Otherwise say “No Update”.
		if not Has_Reading then
			R := (if Time_Now > (End_Of_Sec + Delay_Us_Epsilon)
						then No_Signal else No_Update);
		end if;
		-- If anything (even No_Signal end of second) was output,
		-- forward internal reference by one second.
		if R /= No_Update then
			Ctx.Start_Of_Sec := End_Of_Sec;
		end if;
		return R;
	end Update_Tick;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural is
							(Ctx.Unidentified);
	function Get_Discarded(Ctx: in Bitlayer) return Natural is
							(Ctx.Discarded);
	function Get_Overflown(Ctx: in Bitlayer) return Natural is
							(Ctx.Overflown);
	function Get_Delay(Ctx: in Bitlayer) return Time is (Ctx.Delay_Us);

end DCF77_Bitlayer;
