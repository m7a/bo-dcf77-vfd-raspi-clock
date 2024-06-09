with DCF77_Offsets;
use  DCF77_Offsets;
with DCF77_Functions;
use  DCF77_Functions;

package body DCF77_Minutelayer is

	procedure Init(Ctx: in out Minutelayer) is
	begin
		Ctx.YH                     := TM0.Y / 100;
		Ctx.Preceding_Minute_Ones  := (others => (others => No_Update));
		Ctx.Preceding_Minute_Idx   := Minute_Buf_Idx'Last;
		Ctx.Seconds_Since_Prev     := Unknown;
		Ctx.Seconds_Left_In_Minute := DCF77_Offsets.Sec_Per_Min;
		Ctx.Current                := TM0;
		Ctx.Current_QOS            := QOS9_ASYNC;
		Ctx.Prev_Telegram          := (Valid => Invalid,
						Value => (others => No_Signal));
		Ctx.QOS_Stats              := (others => 0);
	end Init;

	procedure Process(Ctx: in out Minutelayer;
					Has_New_Bitlayer_Signal: in Boolean;
					Telegram_1, Telegram_2: in Telegram;
					Exch: out TM_Exchange) is
	begin
		Exch.Is_Leapsec  := False;
		Exch.Is_New_Sec  := Has_New_Bitlayer_Signal;
		Exch.DST_Delta_H := 0;
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
				Exch.Is_Leapsec := True;
				Ctx.Current.S := 60; -- special leap second case
			else
				if Ctx.Current.S >= Sec_Last_In_Min then
					Ctx.Next_Minute_Coming(Telegram_1,
						Telegram_2, Exch.DST_Delta_H);
				end if;
				Advance_TM_By_Sec(Ctx.Current, 1);
			end if;
		end if;
		if Telegram_1.Valid /= Invalid then
			-- It seems it may not be necessary to set this here
			-- because in theory, New Telegram
			-- => New_Bitlayer_Signal even if that signal is just
			-- of type “NO Signal”. However, for now prefer to
			-- update the clock with New Second here as to assure
			-- that valid telegrams are always processed. This
			-- slightly errors on the side of wrong data but
			-- the overall improvements from the new Timelayer
			-- are expected to reduce errors so much that it may
			-- not matter already. Could revisit this to get out
			-- the last "10%" of the design sometime later...
			--   Actually, already the old implementation only
			-- ever entered this procedure with
			-- Has_New_Bitlayer_Signal = True hence it should all be
			-- OK!
			Exch.Is_New_Sec := True;
			-- End of minute is also new second
			Ctx.Process_New_Telegram(Telegram_1, Telegram_2);
			if Ctx.EOH_DST_Switch = DST_Applied then
				Ctx.EOH_DST_Switch := DST_No_Change;
			end if;
		end if;
		Exch.Proposed     := Ctx.Current;
		Exch.Is_Confident := Ctx.Current_QOS = QOS1;
		-- maintain information about the statistics of the occurrence
		-- of the individual QOS levels
		if Has_New_Bitlayer_Signal then
			if Ctx.QOS_Stats(Ctx.Current_QOS) = Stat_Entry'Last then
				Ctx.QOS_Stats := (others => 0);
			end if;
			Ctx.QOS_Stats(Ctx.Current_QOS) :=
					Ctx.QOS_Stats(Ctx.Current_QOS) + 1;
		end if;
	end Process;

	-- Store out_current as prev if +1 yields a new minute and our current
	-- minute is X9 i.e. next minute will be Y0 with Y = X+1 (or more fields
	-- changed...)
	procedure Next_Minute_Coming(Ctx: in out Minutelayer; Telegram_1,
			Telegram_2: in Telegram; DST_Delta_H: in out Integer) is
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
				DST_Delta_H   := +1;
				Ctx.Current.H := Ctx.Current.H + 1;
			when DST_To_Winter =>
				-- winter = UTC+1, -1h
				DST_Delta_H   := -1;
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

	procedure Process_New_Telegram(Ctx: in out Minutelayer; Telegram_1_In,
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

		Virtual_Telegram: Telegram := TM_To_Telegram(Ctx.Current);
	begin
		Ctx.Add_Minute_Ones_To_Buffer(Telegram_1);
		-- TODO SMALL PROBLEM: THIS IS INCORRECT. HERE, WE DECODE AND
		--      THEN AFTERWARDS KNOW THAT WE JUST SAW A LEAP SECOND.
		--      THIS CAUSES 02:00:60 to be generated in place of the
		--      expected 01:59:60. Test case xe10_201207010144_lp has
		--      the expected data. Might need to reorganize the
		--      minutelayer to account for this. Basically it seems we
		--      would have to “pre-decode” at :59 as to establish wheter
		--      :60 or :00 follows. Alternatively consider some
		--      simplifications...
		Ctx.Seconds_Left_In_Minute := (if Telegram_1.Valid = Valid_60
						then Sec_Per_Min else 61);
		Ctx.Decode_And_Populate_DST_Switch(Telegram_1);

		-- 1a. pre-adjust previous in case we can safely decode it
		if Has_Out_2_Tens then
			Ctx.Prev          := Ctx.Decode_Tens(Telegram_2);
			Ctx.Prev_Telegram := Telegram_2;
		end if;

		-- 2.
		if Out_1_Recovery = Data_Complete then
			if not Ctx.Decode_Check(Telegram_1) and
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
				Intermediate := Ctx.Decode_Tens(Telegram_1);
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
		if Check_If_Compat_By_X_Eliminate(Virtual_Telegram,
								Telegram_1) then
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
			if X_Eliminate_Prev = B_True
					and then Ctx.Try_QOS6(Telegram_1) then
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

	procedure Add_Minute_Ones_To_Buffer(Ctx: in out Minutelayer;
							Tel: in Telegram) is
	begin
		Ctx.Preceding_Minute_Idx := Ctx.Preceding_Minute_Idx + 1;
		Ctx.Preceding_Minute_Ones(Ctx.Preceding_Minute_Idx) :=
				Tel.Value(Offset_Minute_Ones ..
				Offset_Minute_Ones + Length_Minute_Ones - 1);
	end Add_Minute_Ones_To_Buffer;

	procedure Decode_And_Populate_DST_Switch(Ctx: in out Minutelayer;
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

	function Decode_Tens(Ctx: in Minutelayer; Tel: in Telegram) return TM is
		RV: TM := Ctx.Decode(Tel);
	begin
		Discard_Ones(RV);
		return RV;
	end Decode_Tens;

	procedure Discard_Ones(T: in out TM) is
	begin
		T.I := (T.I / 10) * 10;
	end Discard_Ones;

	function Decode(Ctx: in Minutelayer; Tel: in Telegram) return TM is
		Ign_Parity: Parity_State := Parity_Sum_Undefined;

		-- Read and Decode
		function RD(Offset, Length: in Natural) return Natural is
			(Decode_BCD(Tel.Value(Offset .. Offset + Length - 1),
			Ign_Parity));
	begin
		return (Y => Ctx.YH * 100 +
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
	function Decode_Check(Ctx: in out Minutelayer; Tel: in Telegram)
							return Boolean is
		TMN_Interm: constant TM      := Ctx.Decode(Tel);
		RV:         constant Boolean := Ctx.Current = TMN_Interm;
	begin
		Ctx.Current := TMN_Interm;
		return RV;
	end Decode_Check;

	-- @return value if ones were recovered successfully, -1 if not
	function Recover_Ones(Ctx: in out Minutelayer) return Integer is
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

	function Check_If_Compat_By_X_Eliminate(
				Virtual_Telegram: in out Telegram;
				Telegram_1: in Telegram) return Boolean is
		(X_Eliminate(False, Telegram_1, Virtual_Telegram) and then
		-- 0 to not skip anything
		X_Eliminate_Match(Telegram_1, Virtual_Telegram,
				Offset_Minute_Ones, Offset_Minute_Ones +
				Length_Minute_Ones - 1, 0) and then
		BCD_Check_Minute(
			Virtual_Telegram.Value(Offset_Minute_Ones ..
				Offset_Minute_Ones + Length_Minute_Ones - 1),
			Virtual_Telegram.Value(Offset_Minute_Tens ..
				Offset_Minute_Tens + Length_Minute_Tens - 1),
			Virtual_Telegram.Value(Offset_Parity_Minute)
		) = OK);

	function TM_To_Telegram(T: in TM) return Telegram is
		TR: Telegram := TM_To_Telegram_10min(T);
	begin
		WMBC(TR, Offset_Minute_Ones, Length_Minute_Ones, T.I mod 10);
		return TR;
	end TM_To_Telegram;

	-- The value of QOS6 is debatable because it detects a mismatch
	-- by xeliminate and then counts forwards from the prev telegram.
	-- Problem is: In realy, all cases seem to be covered by QOS5.
	-- We leave it enabled for now despite the fact that there is no known
	-- case to “need” this for recovery...
	function Try_QOS6(Ctx: in out Minutelayer; Telegram_1: in Telegram)
							return Boolean is
		Try_Time:         TM := Ctx.Prev;
		Virtual_Telegram: Telegram;
	begin
		-- use prev tens ignore ones
		Discard_Ones(Try_Time);
		-- current = prev tens + 10min + num seconds sin
		Advance_TM_By_Sec(Try_Time, 10 * Sec_Per_Min
						+ Ctx.Seconds_Since_Prev);
		Virtual_Telegram := TM_To_Telegram(Try_Time);

		if Check_If_Compat_By_X_Eliminate(Virtual_Telegram,
								Telegram_1) then
			Ctx.Current            := Try_Time;
			Ctx.Seconds_Since_Prev := Ctx.Seconds_Since_Prev
							+ 10 * Sec_Per_Min;
			Ctx.Current_QOS        := QOS6;
			return True;
		else
			return False;
		end if;
	end Try_QOS6;

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
	function Cross_Check_By_X_Eliminate(Ctx: in out Minutelayer;
				Telegram_1: in out Telegram) return Boolean is
		Current_Minus_One:                 TM := Ctx.Prev;
		Current_Plus_One:                  TM := Ctx.Current;
		Virtual_Telegram_Minus_1_Min:      Telegram;
		Virtual_Telegram_Plus_1_Min:       Telegram;
		Eliminates_For_One_Minute_Back:    Boolean;
		Eliminates_For_One_Minute_Forward: Boolean;
	begin
		Discard_Ones(Current_Minus_One);
		Advance_TM_By_Sec(Current_Minus_One, 9 * Sec_Per_Min +
							Ctx.Seconds_Since_Prev);
		Advance_TM_By_Sec(Current_Plus_One, Sec_Per_Min);

		Virtual_Telegram_Minus_1_Min :=
					TM_To_Telegram(Current_Minus_One);
		Virtual_Telegram_Plus_1_Min :=
					TM_To_Telegram(Current_Plus_One);

		Eliminates_For_One_Minute_Forward :=
				Check_If_Compat_By_X_Eliminate(
				Virtual_Telegram_Plus_1_Min,
				Telegram_1);
		Eliminates_For_One_Minute_Back :=
				Check_If_Compat_By_X_Eliminate(
				Telegram_1, Virtual_Telegram_Minus_1_Min);

		if Eliminates_For_One_Minute_Forward and
					Eliminates_For_One_Minute_Back then
			return False; -- 5C
		elsif Eliminates_For_One_Minute_Back then
			-- 5A
			Ctx.Current            := Current_Minus_One;
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

	function Get_QOS_Sym(Ctx: in Minutelayer) return Character is
		QOS_Descr: constant array (QOS) of Character := (
			QOS1       => '1', QOS2       => '2',
			QOS3       => '3', QOS4       => '4',
			QOS5       => '5', QOS6       => '6',
			QOS7       => '7', QOS8       => '8',
			QOS9_ASYNC => '9'
		);
	begin
		return QOS_Descr(Ctx.Current_QOS);
	end Get_QOS_Sym;

	procedure Set_TM_By_User_Input(Ctx: in out Minutelayer; T: in TM) is
	begin
		-- import data
		Ctx.YH                     := T.Y / 100;
		Ctx.Current                := T;
		Ctx.Seconds_Left_In_Minute := DCF77_Offsets.Sec_Per_Min - T.S;
		-- reset remainder of state
		Ctx.Seconds_Since_Prev     := Unknown;
		Ctx.Current_QOS            := QOS9_ASYNC;
		Ctx.Prev_Telegram.Valid    := Invalid;
		Ctx.Preceding_Minute_Idx   := Minute_Buf_Idx'Last;
	end Set_TM_By_User_Input;

	-- TODO THIS SCREEN IS REALLY NOT USEFUL ANYMORE IT SHOULD BE REPLACED
	--      BY SOMETHING MORE USEFUL (SUGGEST TO USE THREE COUNTERS
	--      P1/PX/P9 -> helps more from experience)
	function Get_QOS_Stats(Ctx: in Minutelayer) return String is
		type Sum_T is mod 2**64;

		S1:   constant Sum_T := Sum_T(Ctx.QOS_Stats(QOS1));
		S23:  constant Sum_T := Sum_T(Ctx.QOS_Stats(QOS2)) +
					Sum_T(Ctx.QOS_Stats(QOS3));
		S45:  constant Sum_T := Sum_T(Ctx.QOS_Stats(QOS4)) +
					Sum_T(Ctx.QOS_Stats(QOS5));
		S678: constant Sum_T := Sum_T(Ctx.QOS_Stats(QOS6)) +
					Sum_T(Ctx.QOS_Stats(QOS7)) +
					Sum_T(Ctx.QOS_Stats(QOS8));
		S9:   constant Sum_T := Sum_T(Ctx.QOS_Stats(QOS9_ASYNC));

		-- +1 avoid div 0 and 100%
		Sum:  constant Sum_T := (S1 + S23 + S45 + S678 + S9) / 100 + 1;

		P1:   constant Natural := Natural(S1   / Sum);
		P23:  constant Natural := Natural(S23  / Sum);
		P45:  constant Natural := Natural(S45  / Sum);
		P678: constant Natural := Natural(S678 / Sum);
		P9:   constant Natural := Natural(S9   / Sum);
	begin
		-- Format TBL >+1 23 45 678  9<
		return  Num_To_Str_L2(P1)  & " " & Num_To_Str_L2(P23)  & " " &
			Num_To_Str_L2(P45) & "  " & Num_To_Str_L2(P678) & " " &
			Num_To_Str_L2(P9);
	end Get_QOS_Stats;

end DCF77_Minutelayer;
