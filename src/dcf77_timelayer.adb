package body DCF77_Timelayer is

	procedure Init(Ctx: in out Timelayer) is
	begin
		Ctx.Preceding_Minute_Ones  := (others => (others => No_Update));
		Ctx.Preceding_Minute_Idx   := Minute_Buf_Idx'Last;
		Ctx.Seconds_Since_Prev     := Prev_Unknown;
		Ctx.Seconds_Left_In_Minute := DCF77_Offsets.Sec_Per_Min;
		Ctx.Current                := T0;
		Ctx.Current_QOS            := QOS9_ASYNC;
		Ctx.Prev_Telegram          := (Valid => Invalid,
						Value => (others => No_Signal));
	end Init;

	procedure Process(Ctx: in out Timelayer;
			Has_New_Bitlayer_Signal: in Boolean;
			Telegram_1, Telegram_2: in DCF77_Secondlayer.Telegram)
			is
	begin
		-- Always handle second if detected
		if Has_New_Bitlayer_Signal then
			-- Add one sec to current time handling leap seconds and
			-- prev management.
			--
			-- Two options:
			-- (a) double compute i.e. first +1sec then see if
			--     telegram processing is compatible if not update.
			-- (b) only compute +1sec if we are within a minute. if
			--     we are at the beginning of a new minute then
			--     _first_ try to process telegram and only revert
			--     to +1 routine if that does not successfully yield
			--     the current time to display.
			--
			-- CURSEL:
			--     (a) double compute to allow setting prev minute
			--     from the model for cases where secondlayer does
			--     not provide us with one. This allows using the
			--     model without the necessity to generate previous
			--     date values.

			if Ctx.Seconds_Since_Prev /= Prev_Unknown then
				Inc_Saturated(Ctx.Seconds_Since_Prev, Prev_Max);
			end if;

			if Ctx.Seconds_Left_In_Minute > 0 then
				Ctx.Seconds_Left_In_Minute :=
						Ctx.Seconds_Left_In_Minute - 1;
			end if;

			if Ctx.Seconds_Left_In_Minute = 1 and
					Ctx.Current.S = Sec_Last_In_Min then
				Ctx.Current.S := 60; -- special leap second case
			else
				if Ctx.Current.S >= Sec_Last_In_Min then
					Ctx.Next_Minute_Coming(Telegram_1,
								Telegram_2);
				end if;
				Advance_TM_By_Sec(Ctx.Current, 1);
			end if;
		end if;
		if Telegram_1.Valid /= Invalid then
			Ctx.Process_New_Telegram(Telegram_1, Telegram_2);
			if Ctx.EOH_DST_Switch = DST_Applied then
				Ctx.EOH_DST_Switch := DST_No_Change;
			end if;
		end if;
	end Process;


	-- Store out_current as prev if +1 yields a new minute and our current
	-- minute is X9 i.e. next minute will be Y0 with Y = X+1 (or more fields
	-- changed...)
	procedure Next_Minute_Coming(Ctx: in out Timelayer; Telegram_1,
				Telegram_2: in DCF77_Secondlayer.Telegram) is
	begin
		-- If this is the only opinion we get here, it essentially means
		-- that we are not in sync!
		Ctx.Current_QOS := QOS9_ASYNC;

		if Ctx.Current.I = Sec_Last_In_Min and
					Ctx.EOH_DST_Switch /= DST_No_Change then
			-- Minor hack: Only supports leap if not across
			-- days i.e. hour changes within day. No checks are
			-- performed. This is the expected convetion in Germany
			-- where we only ever switch between 02:00 (01:59)
			-- and 03:00 (02:59) times.
			case Ctx.EOH_DST_Switch is
			when DST_To_Summer =>
				-- summer = UTF+2, +1h
				Ctx.Current.H := Ctx.Current.H + 1;
			when DST_To_Winter =>
				-- winter = UTC+1, -1h
				Ctx.Current.H := Ctx.Current.H - 1;
			when others =>
				-- ignore
				null;
			end case;
			-- set prev to unknown because now we move more than a
			-- mere ten minute step.
			Ctx.Seconds_Since_Prev := Prev_Unknown;
			Ctx.EOH_DST_Switch     := DST_Applied;
		elsif (Ctx.Current.I mod 10) = 9 then
			Ctx.Seconds_Since_Prev := 0;
			Ctx.Prev_Telegram      := Ctx.Current;
			Ctx.Prev := TM_To_Telegram_10min(Ctx.Prev_Telegram);
		end if;
	end Next_Minute_Needed;

	-- Currently needed to store "prev" values if +1sec yields a new minute
	-- tens. Currently only stores tm to ten minute precision.
	function TM_To_Telegram_10min(T: in TM) return Telegram is
		TR:    Telegram         := (Valid_60, (others => No_Signal));
		Y_Val: constant Natural := T.Y mod 100;
	begin
		Write_Multiple_Bits_Converting(TR, Offset_Year_Ones,
						Length_Year_Ones, Y_Val mod 10);
		Write_Multiple_Bits_Converting(TR, Offset_Year_Tens,
						Length_Year_Tens, Y_Val / 10);
		Write_Multiple_Bits_Converting(TR, Offset_Month_Ones,
						Length_Month_Ones, T.M mod 10);
		Write_Multiple_Bits_Converting(TR, Offset_Month_Tens,
						Length_Month_Tens, T.M / 10);
		Write_Multiple_Bits_Converting(TR, Offset_Day_Oones,
						Length_Day_Ones, T.D mod 10);
		Write_Multiple_Bits_Converting(TR, Offset_Day_Tens,
						Length_Day_Tens, T.D / 10);
		Write_Multiple_Bits_Converting(TR, Offset_Hour_Ones,
						Length_Hour_Ones, T.H mod 10);
		Write_Multiple_Bits_Converting(TR, Offset_Hour_Tens,
						Length_Hour_Tens, T.H / 10);
		Write_Multiple_Bits_Converting(TR, Offset_Minute_Tens,
						Length_Minute_Tens, T.I / 10);
	end TM_To_Telegram_10min;

	procedure Write_Multiple_Bits_Converting(TR: in out Telegram;
					Offset, Length, Value: in Natural) is
		Val_Rem: Natural := Value;
	begin
		-- TODO z I IMPLEMENTED BE CONVERSION AT LEAST ONCE ALREADY
		for I in reverse Offset .. Offset + Length - 1 loop
			TR.Value(I) := (if (Val_Rem mod 2) = 1
					then Bit_1 else Bit_0);
			Val_Rem := Val_Rem / 2;
		end loop;
	end Write_Multiple_Bits_Converting;

	-- Not leap-second aware for now
	-- assert seconds < 12000, otherwise may output incorrect results!
	-- Currently does not work with negative times (implement them for
	-- special DST case by directly changing the hours `i` field).
	procedure Advance_TM_By_Sec(T: in out TM; Seconds: in Natural) is

		-- In case of leap year, access index 0 to return length of 29
		-- days for Feburary in leap years.
		function Get_Month_Length return Natural is
				(Month_Lengths(if (T.M = 2 and
					Is_Leap_Year(T.Y)) then 0 else TM.M));

		Min_Per_Hour:    constant Natural := 60;
		Hours_Per_Day:   constant Natural := 24;
		Months_Per_Year: constant Natural := 12;

		ML: Natural; -- month length cache variable
	begin
		T.S := T.S + Seconds;
		if T.S >= Sec_Per_Min then
			T.I := T.I + T.S /   Sec_Per_Min;
			T.S :=       T.S mod Sec_Per_Min;
			if T.I >= Min_Per_Hour then
				T.H := T.H + T.I /   Min_Per_Hour;
				T.I :=       T.I mod Min_Per_Hour;
				if T.H >= Hours_Per_Day then
					T.D := T.D + T.H /   Hours_Per_Day;
					T.H :=       T.H mod Hours_Per_Day;
					ML  := Get_Month_Length;
					if T.D > ML then
						T.D := T.D - ML;
						T.M := T.M + 1;
						if T.M > Months_Per_Year then
							T.M := 1;
							T.Y := T.Y + 1;
						end if;
					end if;
				end if;
			end if;
		end if;
	end Advance_TM_By_Sec;

	-- https://en.wikipedia.org/wiki/Leap_year
	function Is_Leap_Year(Y: in Natural) return Boolean is (((Y mod 4) = 0)
				and (((Y mod 100) /= 0) or ((Y mod 400) = 0)));

	procedure Process_New_Telegram(Ctx: in out Timelayer; Telegram_1,
						Telegram_2: in Telegram) is
	begin
		null; -- TODO CONTINUE HERE
	end Process_New_Telegram;

	function Get_Current(Ctx: in Timelayer) return TM is (Ctx.Current);

	function Get_Quality_Of_Service(Ctx: in Timelayer) return QOS is
							(Ctx.Current_QOS);

end DCF77_Timelayer;
