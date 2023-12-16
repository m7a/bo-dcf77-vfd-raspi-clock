with DCF77_Offsets;
use  DCF77_Offsets;
with DCF77_Functions;
use  DCF77_Functions;

package body DCF77_Timelayer is

	procedure Init(Ctx: in out Timelayer) is
	begin
		Ctx.Preceding_Minute_Ones  := (others => (others => No_Update));
		Ctx.Preceding_Minute_Idx   := Minute_Buf_Idx'Last;
		Ctx.Seconds_Since_Prev     := Unknown;
		Ctx.Seconds_Left_In_Minute := DCF77_Offsets.Sec_Per_Min;
		Ctx.Current                := TM0;
		Ctx.Current_QOS            := QOS9_ASYNC;
		Ctx.Prev_Telegram          := (Valid => Invalid,
						Value => (others => No_Signal));
	end Init;

	procedure Process(Ctx: in out Timelayer;
					Has_New_Bitlayer_Signal: in Boolean;
					Telegram_1, Telegram_2: in Telegram) is
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

			if Ctx.Seconds_Since_Prev /= Unknown then
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
						Telegram_2: in Telegram) is
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
			Ctx.Seconds_Since_Prev := Unknown;
			Ctx.EOH_DST_Switch     := DST_Applied;
		elsif (Ctx.Current.I mod 10) = 9 then
			Ctx.Seconds_Since_Prev := 0;
			Ctx.Prev               := Ctx.Current;
			Ctx.Prev.S             := 0;
			Ctx.Prev_Telegram := TM_To_Telegram_10min(Ctx.Prev);
		end if;
	end Next_Minute_Coming;

	-- Currently needed to store "prev" values if +1sec yields a new minute
	-- tens. Currently only stores tm to ten minute precision.
	function TM_To_Telegram_10min(T: in TM) return Telegram is
		TR:    Telegram         := (Valid_60, (others => No_Signal));
		Y_Val: constant Natural := T.Y mod 100;
	begin
		WMBC(TR, Offset_Year_Ones,   Length_Year_Ones,   Y_Val mod 10);
		WMBC(TR, Offset_Year_Tens,   Length_Year_Tens,   Y_Val /   10);
		WMBC(TR, Offset_Month_Ones,  Length_Month_Ones,  T.M   mod 10);
		WMBC(TR, Offset_Month_Tens,  Length_Month_Tens,  T.M   /   10);
		WMBC(TR, Offset_Day_Ones,    Length_Day_Ones,    T.D   mod 10);
		WMBC(TR, Offset_Day_Tens,    Length_Day_Tens,    T.D   /   10);
		WMBC(TR, Offset_Hour_Ones,   Length_Hour_Ones,   T.H   mod 10);
		WMBC(TR, Offset_Hour_Tens,   Length_Hour_Tens,   T.H   /   10);
		WMBC(TR, Offset_Minute_Tens, Length_Minute_Tens, T.I   /   10);
		return TR;
	end TM_To_Telegram_10min;

	-- Write Multiple Bits Converting
	procedure WMBC(TR: in out Telegram;
					Offset, Length, Value: in Natural) is
		Val_Rem: Natural := Value;
	begin
		for I in Offset .. Offset + Length - 1 loop
			TR.Value(I) := (if (Val_Rem mod 2) = 1
					then Bit_1 else Bit_0);
			Val_Rem := Val_Rem / 2;
		end loop;
	end WMBC;

	-- Not leap-second aware for now
	-- assert seconds < 12000, otherwise may output incorrect results!
	-- Currently does not work with negative times (implement them for
	-- special DST case by directly changing the hours `i` field).
	procedure Advance_TM_By_Sec(T: in out TM; Seconds: in Natural) is
		-- In case of leap year, access index 0 to return length of 29
		-- days for Feburary in leap years.
		function Get_Month_Length return Natural is
				(Month_Lengths(if (T.M = 2 and
					Is_Leap_Year(T.Y)) then 0 else T.M));

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

	procedure Process_New_Telegram(Ctx: in out Timelayer; Telegram_1_In,
						Telegram_2_In: in Telegram) is
		type Tristate is (Unset, B_True, B_False);
		function B2T(B: in Boolean) return Tristate is
						(if B then B_True else B_False);

		Telegram_1:       Telegram := Telegram_1_In; -- editable copies
		Telegram_2:       Telegram := Telegram_2_In;

		Recovered_Ones:   Integer;
		Intermediate:     TM;

		-- Initialize to "true" value in order to later use this to
		-- detect that xeliminate was not performed yet.
		X_Eliminate_Prev: Tristate := Unset;

		-- 1.
		Out_1_Recovery: constant Recovery := Recover_BCD(Telegram_1);
		Out_2_Recovery: constant Recovery :=
				(if (Telegram_2.Valid /= Invalid)
					then Recover_BCD(Telegram_2)
					else Data_Incomplete_For_Multiple);
		Has_Out_2_Tens: constant Boolean :=
				 Out_2_Recovery = Data_Complete or else
				(Out_2_Recovery = Data_Incomplete_For_Minute
					and then Has_Minute_Tens(Telegram_2));
	begin
		Ctx.Add_Minute_Ones_To_Buffer(Telegram_1);
		Ctx.Seconds_Left_In_Minute := (if Telegram_1.Valid = Valid_60
						then Sec_Per_Min else 61);
		Ctx.Decode_And_Populate_DST_Switch(Telegram_1);

		-- 1a. pre-adjust previous in case we can safely decode it
		if Has_Out_2_Tens then
			Ctx.Prev               := Decode_Tens(Telegram_2);
			Ctx.Prev_Telegram      := Telegram_2;
			-- TODO x likely to have an error here: Would this not
			--        need to be incremented by the current minute
			--        ones * 60 as to account for how long this
			--        “prev” stuff is in the past?
			Ctx.Seconds_Since_Prev := 0;
		end if;

		-- 2.
		if Out_1_Recovery = Data_Complete then
			if not Decode_Check(Ctx.Current, Telegram_1) and
					Out_2_Recovery =
					Data_Incomplete_For_Multiple then
				Ctx.Seconds_Since_Prev := Unknown;
			end if;
			Ctx.Current_QOS := QOS1;
			return;
		end if;

		-- 3.
		Recovered_Ones := Ctx.Recover_Ones;
		if Recovered_Ones /= Unknown then
			-- 3.1 If has ymdhi tens (current),
			--     out1 recovery == complete handled above already
			if Out_1_Recovery = Data_Incomplete_For_Minute and
						Has_Minute_Tens(Telegram_1) then
				Intermediate := Decode_Tens(Telegram_1);
				Advance_TM_By_Sec(Intermediate, Recovered_Ones *
								Sec_Per_Min);
				if Intermediate /= Ctx.Current then
					Ctx.Current := Intermediate;
					-- too deeply nested
					if Out_2_Recovery =
					Data_Incomplete_For_Multiple then
						-- discard pot. invalid prev
						Ctx.Seconds_Since_Prev :=
									Unknown;
					end if;
				end if; -- else is equal and prev remains valid
				Ctx.Current_QOS := QOS2;
				return;
			end if;
			-- 3.2 if has ymdhi tens (prev)
			if Has_Out_2_Tens then
				-- we know that they are already decoded above
				Ctx.Current := Ctx.Prev;
				-- prev + 10min + recovered ones
				Advance_TM_By_Sec(Ctx.Current, 10 * Sec_Per_Min
						+ Recovered_Ones * Sec_Per_Min);
				Ctx.Seconds_Since_Prev :=
						Recovered_Ones * Sec_Per_Min;
				Ctx.Current_QOS := QOS3;
				return;
			end if;
			-- 3.3 xeliminate_prev=xeliminate(secondlayer.prev,prev)
			--
			-- NB: Writes to Prev_Telegram in order to make it as
			--     precise as possible. Need to consider this when
			--     handling subsequent QOS5 case.
			X_Eliminate_Prev := B2T(Telegram_2.Valid /= Invalid
					and then X_Eliminate(False, Telegram_2,
					Ctx.Prev_Telegram));
			if X_Eliminate_Prev = B_True then
				Ctx.Current := Ctx.Prev;
				-- use prev tens ignore ones
				Discard_Ones(Ctx.Current);
				-- current = prev tens + 10min + recoveredOnes
				Advance_TM_By_Sec(Ctx.Current, 10 * Sec_Per_Min
						+ Recovered_Ones * Sec_Per_Min);
				Ctx.Current_QOS := QOS4;
				-- NB: In case X_Eliminate_Prev = B_True i.e.
				-- successful, control flow returns here. From
				-- beyond this return onwards, xeliminate_prev
				-- is B_False iff 3.3 was reached but its
				-- condition failed.
				return;
			end if;
		end if;

		-- 4.
		if Ctx.Check_If_Current_Compat_By_X_Eliminate(Telegram_1) then
			-- QOS5: The automatically computed time is compatible
			-- with the received (possibly incomplete) telegram.
			-- Hence we now run on our own clock but know that the
			-- data is at least not contrary to the signals received
			-- despite the fact that we currently cannot decode them
			-- properly.
			Ctx.Current_QOS := QOS5;
			return;
		end if;

		-- 5.
		if Ctx.Seconds_Since_Prev /= Unknown then
			if X_Eliminate_Prev = Unset then
				-- means we did not do xeliminate for prev yet,
				-- do it now
				X_Eliminate_Prev := B2T(X_Eliminate(False,
						Telegram_2, Ctx.Prev_Telegram));
			end if;
			-- TODO PROBLEM: THIS CAN INDEED PRODUCE DATA IN
			--      CONFLICT WITH WHAT WE RECEIVED. WE MUST NEVER
			--      OUTPUT SOMETHING “KNOWINGLY WRONG”. HENCE NEED
			--      TO DO SOMETHING LIKE THE CURRENT COMPAT BY
			--      XELIMINATE WITH THE CONSTRUCTED CURRENT AND NOT
			--      BLINDLY OVERWRITE IT JUST YET.
			if X_Eliminate_Prev = B_True then
				Ctx.Current := Ctx.Prev;
				-- use prev tens ignore ones
				Discard_Ones(Ctx.Current);
				-- current = prev tens + 10min + num seconds sin
				Advance_TM_By_Sec(Ctx.Current, 10 * Sec_Per_Min
						+ Ctx.Seconds_Since_Prev);
				Ctx.Seconds_Since_Prev := Ctx.Seconds_Since_Prev
						+ 10 * Sec_Per_Min;
				Ctx.Current_QOS := QOS6;
				return;
			end if;
			-- 5ABC: Spot misalignments.
			-- If xeliminate_prev fails, then do some cross checks
			if Ctx.Prev_Telegram.Valid /= Invalid and then
			Ctx.Cross_Check_By_X_Eliminate(Telegram_1) then
				-- Time adjustments performed in
				-- cross_check_by_xeliminate
				return;
			end if;
		end if;

		-- Nothing more to try, now runs async.
		Ctx.Current_QOS := QOS9_ASYNC;
	end Process_New_Telegram;

	function Recover_BCD(Tel: in out Telegram) return Recovery is

		function Read_Multiple(Offset, Length: in Natural) return Bits
				is (Tel.Value(Offset .. Offset + Length - 1));

		-- Attempts to recover single bit errors by using the last bit
		-- in the offset/len range as parity. If any two bits are
		-- NO_SIGNAL then recovery is impossible and data is left as-is.
		--
		-- @return False if at least one of the data bits (all in range
		-- offset..len-2) is undefined. 1 if all data bits are defined.
		function Recover_Bit(Offset, Length: in Natural)
							return Boolean is
			Known_Missing: Integer := Unknown;
			-- counts number of ones in sequence
			Parity: Natural := 0;
		begin
			for I in Offset .. Offset + Length - 1 loop
				case Tel.Value(I) is
				when No_Signal =>
					if Known_Missing = Unknown then
						-- It's the first no signal.
						-- Store position.
						Known_Missing := I;
					else
						-- More than one bit missing.
						-- Cannot recover.
						return False;
					end if;
				when Bit_1 =>
					Parity := Parity + 1;
				when others =>
					-- pass, do not update parity
					--
					-- Having No_Update here would be an
					-- error but cannot be handled here!
					null;
				end case;
			end loop;
			if (Parity mod 2) = 0 then
				-- Even parity is accepted hence recover
				-- potential missing bit to 0.
				if Known_Missing /= Unknown then
					Tel.Value(Known_Missing) := Bit_0;
				end if;
				-- In any case this means parity passed!
				return True;
			elsif Known_Missing = Unknown then
				-- Odd number of ones means there must be
				-- missing something. However, if we do not have
				-- an index for it this it means the data is
				-- bad. This is an error case!
				--
				-- Say "recovery failed" to indicate that
				-- something is fishy and the data better not be
				-- trusted from the segment of interest.
				return False;
			else
				-- Odd number of ones and missing index exists
				-- means we recover the missing bit to 1.
				Tel.Value(Known_Missing) := Bit_1;
				return True;
			end if;
		end Recover_Bit;

		Ign_Parity: Parity_State := Parity_Sum_Undefined;
		RV: Recovery := Data_Complete;
	begin
		-- 21-28: Minute recovery
		-- We know that 111 (7) for minute tens is impossible.
		-- => Recover 11X to 110.
		pragma Warnings(Off, "condition is always True");
		Drain_Assert(Static_Assert'(Length_Minute_Tens = 3));
		pragma Warnings(On, "condition is always True");
		if Read_Multiple(Offset_Minute_Tens, Length_Minute_Tens) =
						(Bit_1, Bit_1, No_Signal) then
			Tel.Value(Offset_Minute_Tens + 2) := Bit_0;
		end if;
		-- after that, recover single bit errors if exactly one remains.
		if not Recover_Bit(Offset_Minute_Ones, 8) then
			RV := Data_Incomplete_For_Minute;
		end if;

		-- 29-35: Hour recovery
		-- We know that 11 (3) for hour tens is impossible.
		-- => Recover 1X to 10.
		if Tel.Value(Offset_Hour_Tens) = Bit_1 then
			Tel.Value(Offset_Hour_Tens + 1) := Bit_0;
		end if;
		-- after that, recover single bit errors...
		if not Recover_Bit(Offset_Hour_Ones, 7) then
			RV := Data_Incomplete_For_Multiple;
		end if;

		-- 36-58: Date recovery
		-- We know that DOW value 000 is invalid.
		-- => Recover 00X to 001.
		pragma Warnings(Off, "condition is always True");
		Drain_Assert(Static_Assert'(Length_Day_Of_Week = 3));
		pragma Warnings(On, "condition is always True");
		if Read_Multiple(Offset_Day_Of_Week, Length_Day_Of_Week) =
						(Bit_0, Bit_0, No_Signal) then
			Tel.Value(Offset_Day_Of_Week + 2) := Bit_1;
		end if;

		-- We know that if month ones are > 2 then month tens must be 0.
		-- I.e. If month ones has 4-bit or higher set OR
		--      both lower bits set then recover month tens to 0.
		if Tel.Value(Offset_Month_Tens) = No_Signal and then
				Decode_BCD(Read_Multiple(Offset_Month_Ones,
					Length_Month_Ones), Ign_Parity) > 2 then
			Tel.Value(Offset_Month_Tens) := Bit_0;
		end if;

		-- After that, recover single bit errors...
		if not Recover_Bit(Offset_Day_Ones, 23) then
			RV := Data_Incomplete_For_Multiple;
		end if;

		return RV;
	end Recover_BCD;

	procedure Drain_Assert(X: in Boolean) is null;

	function Has_Minute_Tens(Tel: in Telegram) return Boolean is
	begin
		for V of Tel.Value(Offset_Minute_Tens .. Offset_Minute_Tens +
						Length_Minute_Tens - 1) loop
			if V = No_Signal then
				return False; -- fail if at least one missing
			end if;
		end loop;
		return True;
	end Has_Minute_Tens;

	procedure Add_Minute_Ones_To_Buffer(Ctx: in out Timelayer;
							Tel: in Telegram) is
	begin
		Ctx.Preceding_Minute_Idx := Ctx.Preceding_Minute_Idx + 1;
		Ctx.Preceding_Minute_Ones(Ctx.Preceding_Minute_Idx) :=
				Tel.Value(Offset_Minute_Ones ..
				Offset_Minute_Ones + Length_Minute_Ones - 1);
	end Add_Minute_Ones_To_Buffer;

	procedure Decode_And_Populate_DST_Switch(Ctx: in out Timelayer;
							Tel: in Telegram) is
		Announce:   constant Reading := Tel.Value(Offset_DST_Announce);
		-- It is either 10 = currently summer time
		--       xor    01 = currently winter time
		Current_TZ: constant Bits    := Tel.Value(
					Offset_Daylight_Saving_Time ..
					Offset_Daylight_Saving_Time +
					Length_Daylight_Saving_Time - 1);
	begin
		pragma Warnings(Off, "condition is always True");
		Drain_Assert(Static_Assert'(Length_Daylight_Saving_Time = 2));
		pragma Warnings(On, "condition is always True");

		if Announce = Bit_0 then
			Ctx.EOH_DST_Switch := DST_No_Change;
		elsif Announce = Bit_1 and
					Ctx.EOH_DST_Switch /= DST_Applied then
			-- if we have just applied the switch, then do not
			-- propose it for the next minute.
			if Current_TZ = (Bit_1, Bit_0) then -- have summer
				Ctx.EOH_DST_Switch := DST_To_Winter;
			elsif Current_TZ = (Bit_0, Bit_1) then -- have winter
				Ctx.EOH_DST_Switch := DST_To_Summer;
			end if; -- else ignore if not specified exactly;
		end if; -- else ignore if no data available
	end Decode_And_Populate_DST_Switch;

	function Decode_Tens(Tel: in Telegram) return TM is
		RV: TM := Decode(Tel);
	begin
		Discard_Ones(RV);
		return RV;
	end Decode_Tens;

	procedure Discard_Ones(T: in out TM) is
	begin
		T.I := (T.I / 10) * 10;
	end Discard_Ones;

	function Decode(Tel: in Telegram) return TM is
		Ign_Parity: Parity_State := Parity_Sum_Undefined;

		-- Read and Decode
		function RD(Offset, Length: in Natural) return Natural is
			(Decode_BCD(Tel.Value(Offset .. Offset + Length - 1),
			Ign_Parity));
	begin
		return (Y => (TM0.Y / 100) * 100 +
			     RD(Offset_Year_Tens,   Length_Year_Tens) * 10 +
			     RD(Offset_Year_Ones,   Length_Year_Ones),
			M => RD(Offset_Month_Tens,  Length_Month_Tens) * 10 +
			     RD(Offset_Month_Ones,  Length_Month_Ones),
			D => RD(Offset_Day_Tens,    Length_Day_Tens) * 10 +
			     RD(Offset_Day_Ones,    Length_Day_Ones),
			H => RD(Offset_Hour_Tens,   Length_Hour_Tens) * 10 +
			     RD(Offset_Hour_Ones,   Length_Hour_Ones),
			I => RD(Offset_Minute_Tens, Length_Minute_Tens) * 10 +
			     RD(Offset_Minute_Ones, Length_Minute_Ones),
			S => 0);
	end Decode;

	-- @return true if no differences were detected
	function Decode_Check(Current: in out TM; Tel: in Telegram)
							return Boolean is
		TMN_Interm: constant TM      := Decode(Tel);
		RV:         constant Boolean := Current = TMN_Interm;
	begin
		Current := TMN_Interm;
		return RV;
	end Decode_Check;

	-- @return value if ones were recovered successfully, -1 if not
	function Recover_Ones(Ctx: in out Timelayer) return Integer is
		Found_Idx:     Integer := Unknown;
		Idx_Preceding: Minute_Buf_Idx := Ctx.Preceding_Minute_Idx;
		Idx_Delta:     Minute_Buf_Idx;
		Idx_Pass: array (Minute_Buf_Idx) of Boolean := (others => True);
	begin
		for Idx_Compare in Minute_Buf_Idx'Range loop
			Idx_Delta := Idx_Compare;
			loop
				if not Are_Ones_Compatible(
					BCD_Comparison_Sequence(Idx_Delta),
					Ctx.Preceding_Minute_Ones(Idx_Preceding)
				) then
					Idx_Pass(Idx_Compare) := False;
					Idx_Preceding :=
						Ctx.Preceding_Minute_Idx;
					exit;
				end if;

				Idx_Preceding := Idx_Preceding + 1;
				Idx_Delta     := Idx_Delta     + 1;

				exit when Idx_Preceding =
						Ctx.Preceding_Minute_Idx;
			end loop;
		end loop; 
		for Idx_Compare in Minute_Buf_Idx'Range loop
			if Idx_Pass(Idx_Compare) then
				if Found_Idx = Unknown then
					Found_Idx := Integer(Idx_Compare);
				else
					-- another match => not unique
					return Unknown;
				end if;
			end if;
		end loop;
		return Found_Idx;
	end Recover_Ones; 

	-- Like xeliminate but without changing the values:
	-- Returns 1 if ones0 and ones1 could denote the same number.
	-- Returns 0 if they must represent different numbers.
	function Are_Ones_Compatible(AD, BD: in BCD_Digit) return Boolean is
		function Is_Bit_Conflict(A, B: in Reading) return Boolean is
						((A = Bit_0 and B = Bit_1) or
						 (A = Bit_1 and B = Bit_0));
	begin
		for I in BCD_Digit'Range loop
			if Is_Bit_Conflict(AD(I), BD(I)) then
				return False;
			end if;
		end loop;
		return True;
	end Are_Ones_Compatible;

	function Check_If_Current_Compat_By_X_Eliminate(Ctx: in out Timelayer;
				Telegram_1: in Telegram) return Boolean is
		Virtual_Telegram: Telegram := TM_To_Telegram(Ctx.Current);
	begin
		return
			X_Eliminate(False, Telegram_1, Virtual_Telegram)
		and then
			-- 0 to not skip anything
			X_Eliminate_Match(Telegram_1, Virtual_Telegram,
				Offset_Minute_Ones, Offset_Minute_Ones +
				Length_Minute_Ones - 1, 0)
		and then
			BCD_Check_Minute(
				Virtual_Telegram.Value(Offset_Minute_Ones ..
						Offset_Minute_Ones +
							Length_Minute_Ones - 1),
				Virtual_Telegram.Value(Offset_Minute_Tens ..
						Offset_Minute_Tens +
							Length_Minute_Tens - 1),
				Virtual_Telegram.Value(Offset_Parity_Minute)
			) = OK;
	end Check_If_Current_Compat_By_X_Eliminate;

	function TM_To_Telegram(T: in TM) return Telegram is
		TR: Telegram := TM_To_Telegram_10min(T);
	begin
		WMBC(TR, Offset_Minute_Ones, Length_Minute_Ones, T.I mod 10);
		return TR;
	end TM_To_Telegram;

	-- 5A: xeliminate(secondlayer current, ctx prev) if matches
	--     then let time = ctx prev.
	-- 5B: xeliminate(secondlayer current, ctx current + 1min) if matches
	--     then let time = ctx current + 1min
	-- 5C: if both are true than DO NOT DO ANYTHING because this means
	--     xeliminates might just consist of "anything" telegrams that
	--     do not specify enough data to conclude anything from it!
	--
	-- @param Telegram_1 is in out but output is never processed by
	--        outside procedure. It only saves us from performing an
	--        additional copy of the input.
	function Cross_Check_By_X_Eliminate(Ctx: in out Timelayer;
				Telegram_1: in out Telegram) return Boolean is
		Current_Plus_One:                  TM := Ctx.Current;
		Virtual_Telegram_Plus_1_Min:       Telegram;
		Eliminates_For_One_Minute_Back:    Boolean;
		Eliminates_For_One_Minute_Forward: Boolean;
	begin
		Advance_TM_By_Sec(Current_Plus_One, Sec_Per_Min);
		Virtual_Telegram_Plus_1_Min := TM_To_Telegram(Current_Plus_One);

		Eliminates_For_One_Minute_Forward := X_Eliminate(False,
				Telegram_1, Virtual_Telegram_Plus_1_Min);
		Eliminates_For_One_Minute_Back := X_Eliminate(False,
				Ctx.Prev_Telegram, Telegram_1);

		if Eliminates_For_One_Minute_Forward and
					Eliminates_For_One_Minute_Back then
			return False; -- 5C
		elsif Eliminates_For_One_Minute_Back then
			-- 5A
			-- TODO PROBLEM: PREV CAN BE 10min AWAY NOT JUST ONE.
			--      HENCE NO WAY TO DO IT THIS WAY HERE!!!
			--      WRONG CODE!
			Ctx.Current            := Ctx.Prev;
			Ctx.Seconds_Since_Prev := Unknown;
			Ctx.Current_QOS        := QOS7; -- backward
			return True;
		elsif Eliminates_For_One_Minute_Forward then
			-- 5B
			Ctx.Current            := Current_Plus_One;
			Ctx.Seconds_Since_Prev := Unknown;
			Ctx.Current_QOS        := QOS8; -- forward
			return True;
		else
			-- none of the 5-series cases match (<=> both fail)
			return False;
		end if;
	end Cross_Check_By_X_Eliminate;

	function Get_Current(Ctx: in Timelayer) return TM is (Ctx.Current);

	function Get_Quality_Of_Service(Ctx: in Timelayer) return QOS is
							(Ctx.Current_QOS);

end DCF77_Timelayer;
