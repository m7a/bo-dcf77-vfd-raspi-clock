with DCF77_Functions; -- Inc_Saturated
use  DCF77_Functions;

package body DCF77_Timelayer is

	procedure Init(Ctx: in out Timelayer) is
	begin
		Ctx.DCF77_Enabled                   := True;
		Ctx.Prefer_Seconds_From_Minutelayer := True;
		Ctx.Is_Init                         := True;
		Ctx.Last                            := Time_Of_Compilation;
		Ctx.Ctr                             := 0;
		Ctx.QOS_Sym                         := 'I';
		Ctx.Before                          := Time_Of_Compilation;
		Init(Ctx.QOS_Record);
	end Init;

	procedure Process(Ctx: in out Timelayer; Exch: in TM_Exchange) is
	begin
		-- No update if no update
		if Exch.Is_New_Sec then
			Ctx.Process_New_Sec(Exch);
		end if;
	end Process;

	procedure Process_New_Sec(Ctx: in out Timelayer;
							Exch: in TM_Exchange) is
		Old: constant TM := Ctx.Before;

		-- There is also the case of seconds going back that indicates
		-- that the minutelayer suggests to set a time not consistent
		-- with what we have observed before. We treat this like
		-- “non equal” although in such cases, of course, minutes are
		-- indeed equal to -- before.
		Min_EQ: constant Boolean :=
				Are_Minutes_Equal(Exch.Proposed, Ctx.Last) and
				Exch.Proposed.S > Ctx.Last.S;
	begin
		-- In event that we output a leap second inc by “0” is
		-- equivalent to increasing the leap second enabled timestamp
		-- by 1 because 60 div 60 = 1 and 60 mod 60 = 0 -> increments
		-- minute just fine...
		Advance_TM_By_Sec(Ctx.Before,
					(if Ctx.Before.S = 60 then 0 else 1));

		-- If not enabled just count the seconds dumbly...
		if Ctx.DCF77_Enabled then
			if Min_EQ then
				Ctx.Last := Exch.Proposed;
				-- Nothing significant changed: Only updates in
				-- the seconds. Output is per what we prefer
				if Ctx.Prefer_Seconds_From_Minutelayer then
					Ctx.Before := Ctx.Last;
				end if;
				-- Just so you know we leave QOS as-is
			else
				-- Else do real processing here
				Ctx.Process_New_Minute(Exch, Old);
			end if;
		else
			-- Effectively the outcome is ignored, but we
			-- still maintain the counter s.t. we can switch
			-- back to DCF77 processing more easily later.
			if Min_EQ then
				Ctx.Last := Exch.Proposed;
			else
				Ctx.Set_Last_And_Count(Exch.Proposed);
			end if;
		end if;

		case Ctx.QOS_Sym is
		when '+'    => Ctx.QOS_Record.Inc(Good);
		when 'o'    => Ctx.QOS_Record.Inc(Intermediate);
		when others => Ctx.QOS_Record.Inc(Bad);
		end case;
	end Process_New_Sec;

	-- A comparison that ignores the seconds value
	function Are_Minutes_Equal(A, B: in TM) return Boolean is
				(A.Y = B.Y and A.I = B.I and A.D = B.D and
				A.H = B.H and A.I = B.I);

	procedure Process_New_Minute(Ctx: in out Timelayer;
					Exch: in TM_Exchange; Old: in TM) is
		Next: TM;
	begin
		if Ctx.Is_Init then
			Ctx.Set_Last_And_Count(Exch.Proposed);
			Ctx.Before := Exch.Proposed;
			if Exch.Is_Confident or Ctx.Ctr > 10 then
				Ctx.QOS_Sym                         := '+';
				Ctx.Prefer_Seconds_From_Minutelayer := True;
				Ctx.Is_Init                         := False;
			end if;
		else
			-- If confident and matches what we know for hours and
			-- minutes may apply DST switch to output directly.
			if Exch.Is_Confident then
				if Exch.DST_Delta_H = 1
						and Ctx.Before.H = 2
						and Ctx.Before.I = 0
						and Ctx.Before.S = 0 then
					Ctx.Before.H := 3;
				elsif Exch.DST_Delta_H = -1
						and Ctx.Before.H = 3
						and Ctx.Before.I = 0
						and Ctx.Before.S = 0 then
					Ctx.Before.H := 2;
				end if;
			end if;

			Next := Ctx.Before;
			Advance_TM_By_Sec(Next, 60);

			Ctx.Set_Last_And_Count(Exch.Proposed);

			if Ctx.Ctr > 3 or Ctx.Last = Ctx.Before then
				-- QOS 1--3
				Ctx.Prefer_Seconds_From_Minutelayer := True;
				Ctx.Before                          := Ctx.Last;
				Ctx.QOS_Sym                         := '+';
			elsif Exch.Is_Confident and (Ctx.Last = Old or
							Ctx.Last = Next) then
				-- QOS 4a/4b
				Ctx.Prefer_Seconds_From_Minutelayer := True;
				Ctx.Before                          := Ctx.Last;
				Ctx.QOS_Sym                         := 'o';
			else
				-- QOS9, use CTX.Before as computed
				Ctx.Prefer_Seconds_From_Minutelayer := False;
				Ctx.QOS_Sym                         := '-';
			end if;
		end if;
	end Process_New_Minute;

	procedure Set_Last_And_Count(Ctx: in out Timelayer; Proposed: in TM) is
		Virtual_Next: TM := Ctx.Last;
	begin
		Advance_TM_By_Sec(Virtual_Next, 1);
		if Virtual_Next = Proposed then
			Inc_Saturated(Ctx.Ctr, 9999);
		else
			Ctx.Ctr := 0;
		end if;
		Ctx.Last := Proposed;
	end Set_Last_And_Count;

	function Get_Current(Ctx: in Timelayer) return TM is (Ctx.Before);
	function Get_QOS_Sym(Ctx: in Timelayer) return Character is
							(Ctx.QOS_Sym);

	procedure Set_TM_By_User_Input(Ctx: in out Timelayer; T: in TM) is
	begin
		Ctx.Last   := T;
		Ctx.Ctr    := 0;
		Ctx.Before := T;
	end Set_TM_By_User_Input;

	function Is_DCF77_Enabled(Ctx: in Timelayer) return Boolean is
							(Ctx.DCF77_Enabled);

	procedure Set_DCF77_Enabled(Ctx: in out Timelayer; En: in Boolean) is
	begin
		Ctx.DCF77_Enabled := En;
		-- Upon enable, do not immediately take seconds from minutelayer
		-- rather let the system use CTR to determine that.
		if not Ctx.DCF77_Enabled then
			Ctx.Prefer_Seconds_From_Minutelayer := False;
			Ctx.QOS_Sym                         := '-';
		end if;
	end Set_DCF77_Enabled;

	function Get_QOS_Stats(Ctx: in Timelayer) return String is
						(Ctx.QOS_Record.Format_Report);

end DCF77_Timelayer;
