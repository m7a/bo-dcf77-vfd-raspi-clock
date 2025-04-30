with DCF77_TM_Layer_Shared;
use  DCF77_TM_Layer_Shared;

package body DCF77_Alarm is

	----------------------------------------------------[ Implementation ]--

	procedure Init(Ctx: in out Alarm; LLI: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL                := LLI;
		Ctx.S                 := AL_Start;
		Ctx.DT_Now            := (others => 0);
		Ctx.DT_Stop_Buzz      := (others => 0);
		Ctx.T_AL              := (others => 0);
		Ctx.Blink_CTR         := 0;
		Ctx.AL_Has_Changed    := False;
		Ctx.Trace_Button_Down := (others => 0);
		Ctx.Trace_Button_Up   := (others => 0);
		Ctx.Trace_Alarm_Fired := (others => 0);
	end Init;

	procedure Process(Ctx: in out Alarm; DT_Now_Prime: in TM) is
		Is_EN: constant Boolean := Ctx.LL.Read_Alarm_Switch_Is_Enabled;
	begin
		case Ctx.S is
		when AL_Start =>
			-- When enabled in AL_Start means enabled after power
			-- off. This could be a power outrage. Wake the user
			-- because their originally intended wakeup time is not
			-- known and only the bool info that they wanted to be
			-- notified remains as encoded by the switch setting...
			if Is_EN then
				Ctx.Trace_Button_Down := DT_Now_Prime;
				Ctx.Start_Buzzing(DT_Now_Prime);
			else
				Ctx.Trace_Button_Up := DT_Now_Prime;
				Ctx.S               := AL_Disabled;
			end if;
		when AL_Buzz =>
			if not Is_EN then
				Ctx.LL.Set_Buzzer_Enabled(False);
				Ctx.LL.Set_Alarm_LED_Enabled(False);
				Ctx.Trace_Button_Up := DT_Now_Prime;
				Ctx.S               := AL_Disabled;
			elsif DT_Now_Prime >= Ctx.DT_Stop_Buzz then
				Ctx.LL.Set_Buzzer_Enabled(False);
				Ctx.S := AL_Timeout;
			else
				Ctx.Run_Blink;
			end if;
		when AL_Timeout =>
			if Is_EN then
				Ctx.Run_Blink;
			else
				Ctx.LL.Set_Alarm_LED_Enabled(False);
				Ctx.S := AL_Disabled;
			end if;
		when AL_Disabled =>
			if Is_EN then
				Ctx.LL.Set_Alarm_LED_Enabled(True);
				Ctx.DT_Now            := DT_Now_Prime;
				Ctx.Trace_Button_Down := DT_Now_Prime;
				Ctx.S                 := AL_Check;
			end if;
		when AL_Check =>
			if Is_EN then
				if Ctx.AL_Has_Changed then
					Ctx.DT_Now := DT_Now_Prime;
				end if;
				if Ctx.Check_For_Time_Of_Alarm(DT_Now_Prime)
				then
					Ctx.Start_Buzzing(DT_Now_Prime);
				end if;
			else
				Ctx.LL.Set_Alarm_LED_Enabled(False);
				Ctx.Trace_Button_Up := DT_Now_Prime;
				Ctx.S               := AL_Disabled;
			end if;
		end case;
		-- reset even if was not relevant...
		Ctx.AL_Has_Changed := False;
	end Process;

	procedure Start_Buzzing(Ctx: in out Alarm; DT_Now_Prime: in TM) is
	begin
		Ctx.LL.Set_Buzzer_Enabled(True);
		Ctx.Blink_CTR := 0; -- Buzzing always implies blinking!
		-- compute time to stop at the latest if no user interaction
		-- happens in the meantime!
		Ctx.DT_Stop_Buzz := DT_Now_Prime;
		Advance_TM_By_Sec(Ctx.DT_Stop_Buzz, Buzz_Timeout_Seconds);
		Ctx.Trace_Alarm_Fired := DT_Now_Prime;
		Ctx.S := AL_Buzz;
	end Start_Buzzing;

	procedure Run_Blink(Ctx: in out Alarm) is
	begin
		Ctx.LL.Set_Alarm_LED_Enabled(Ctx.Blink_CTR > 2);
		Ctx.Blink_CTR := Ctx.Blink_CTR + 1;
	end Run_Blink;

	function Check_For_Time_Of_Alarm(Ctx: in out Alarm;
					DT_Now_Prime: in TM) return Boolean is
		-- (1) TIME(dtNow') >= tAL equiv
		--	Current timestamp is after configured AL time
		Time_DT_Now_Prime_GE_T_AL: constant Boolean :=
					Time(DT_Now_Prime) >= Ctx.T_AL;

		-- (2) tAl >= TIME(dtNow) equiv
		--	Alarm was configured at the same day that it is
		--	intended to trigger on.
		T_AL_GE_Time_DT_Now: constant Boolean :=
					Ctx.T_AL >= Time(Ctx.DT_Now);
		-- (3) DATE(dtNow) /= DATE(dtNow') equiv
		--	Day has changed compared to when AL was set
		Date_DT_Now_Neq_Now_Prime: constant Boolean := 
					Date(Ctx.DT_Now) /= Date(DT_Now_Prime);

		-- [C1] “The 23:59” case, not needed in permanent QOS1.
		-- If an alarm was configured for 23:59 but that time was
		-- never reached (say we were QOS9_ASYNC at 23:58 and then
		-- found out that the current time is actually next day 00:00),
		-- then none of the other rules triggers and the alarm would
		-- be missed. Hence declare that if at the time of configuring
		-- the alarm it was on the same day (2) and also the date has
		-- changed (3) the alarm must have been missed and should be
		-- triggered right now.
		C1: constant Boolean := T_AL_GE_Time_DT_Now and
					Date_DT_Now_Neq_Now_Prime;

		-- [C2] The same day case “remind me in 5min”.
		-- When at the time of configuring the declared time was in
		-- the future on the same day (2) and also current time is
		-- beyond that time (1) it means the alarm is to be triggered!
		C2: constant Boolean := Time_DT_Now_Prime_GE_T_AL and
					T_AL_GE_Time_DT_Now;

		-- [C3] The other day case “Tomorrow at 09:00 AM”
		-- When at the time of configuring the declared time was in the
		-- past it means it was intended to set off the alarm on the
		-- next day. This can be modelled by the date having changed (3)
		-- and the time being beyond the set time (1)
		C3: constant Boolean := Time_DT_Now_Prime_GE_T_AL and
					Date_DT_Now_Neq_Now_Prime;

		-- [C4] The DST switch case.
		-- I specify a time seemingly in the past but due to DST switch
		-- it is less than one hour away. This will be recognized as the
		-- time moving backwards (4) and if that is the case it is
		-- sufficent -- for the time to be “after” the set time (1)
		-- irrespective of the date component.
		C4: constant Boolean := Time_DT_Now_Prime_GE_T_AL and
					-- (4): Time moves backwards, e.g.
					--      Summer (+2) to Winter (+1) switch
					DT_Now_Prime < Ctx.DT_Now;
	begin
		return C1 or C2 or C3 or C4;
	end Check_For_Time_Of_Alarm;

	function ">="(A, B: in TM) return Boolean is
		(A.Y > B.Y or else
		(A.Y = B.Y and then
			(A.M > B.M or else
			(A.M = B.M and then
				(A.D > B.D or else
				(A.D = B.D and then
					(A.H > B.H or else
					(A.H = B.H and then
						(A.I > B.I or else
						(A.I = B.I and then
							A.S >= B.S
						))
					))
				))
			))
		));

	function "<"(A, B: in TM) return Boolean is (not (A >= B));

	function Date(T: in TM) return Date_T is (Y => T.Y, M => T.M, D => T.D);
	function Time(T: in TM) return Time_T is (H => T.H, I => T.I, S => T.S);

	function ">="(A, B: in Time_T) return Boolean is
		(A.H > B.H or else
		(A.H = B.H and then
			(A.I > B.I or else
			(A.I = B.I and then (A.S >= B.S)))
		));

	---------------------------------------------------------[ Interface ]--

	function Get_AL_Time(Ctx: in Alarm) return Time_T is (Ctx.T_AL);

	function Is_Alarm_Enabled(Ctx: in Alarm) return Boolean is
							(Ctx.S /= AL_Disabled);

	procedure Set_AL_Time(Ctx: in out Alarm; T: in Time_T) is
	begin
		Ctx.T_AL           := T;
		Ctx.AL_Has_Changed := True;
	end Set_AL_Time;

	function Get_Trace_Button_Down(Ctx: in Alarm) return TM
						is (Ctx.Trace_Button_Down);
	function Get_Trace_Button_Up(Ctx: in Alarm) return TM
						is (Ctx.Trace_Button_Up);
	function Get_Trace_Alarm_Fired(Ctx: in Alarm) return TM
						is (Ctx.Trace_Alarm_Fired);

end DCF77_Alarm;
