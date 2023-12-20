package body DCF77_ST_Layer_Shared is

	function X_Eliminate(Telegram_1_Is_Leap: in Boolean;
				Telegram_1: in Telegram;
				Telegram_2: in out Telegram) return Boolean is

		function Begin_Of_Minute return Inner_Checkresult is
		begin
			if X_Eliminate_Entry(
				Telegram_1.Value(Offset_Begin_Of_Minute),
				Telegram_2.Value(Offset_Begin_Of_Minute))
			then
				case Telegram_2.Value(Offset_Begin_Of_Minute) is
				when Bit_1 =>
					return Error_3; -- constant 0 violated
				when No_Signal =>
					-- correct to 0
					Telegram_2.Value(Offset_Begin_Of_Minute)
								:= Bit_0;
					return OK;
				when others =>
					return OK;
				end case;
			else
				return Error_2;
			end if;
		end Begin_Of_Minute;

		function DST_Announce return Inner_Checkresult is
		begin
			X_Eliminate_Entry(Telegram_1.Value(Offset_DST_Announce),
					Telegram_2.Value(Offset_DST_Announce));
			-- The interesting thing about DST_Announce is that it
			-- can persist one minute after the actual change. Hence
			-- we really cannot use this for conflict recognition
			-- and hence always return OK and only take value from
			-- preceding telegram to replace current one as
			-- necessary but never report anything about perceived
			-- conflicts...
			return OK;
		end DST_Announce;

		function Match(From, To, Except: in Natural)
					return Inner_Checkresult is
			(if X_Eliminate_Match(Telegram_1, Telegram_2,
					From, To, Except) then OK else Error_4);

		-- 17+18: needs to be 10 or 01
		function Daylight_Saving_Time return Inner_Checkresult is
			DST1: constant Reading := Telegram_2.Value(
					Offset_Daylight_Saving_Time);
			DST2: constant Reading := Telegram_2.Value(
					Offset_Daylight_Saving_Time + 1);
		begin
			-- assertion violated if 00 or 11 found
			if (DST1 = Bit_0 and DST2 = Bit_0) or
					(DST1 = Bit_1 and DST2 = Bit_1) then
				return Error_5;
			end if;
			if DST2 = No_Signal and DST1 /= No_Signal and
							DST1 /= No_Update then
				-- use 17 to infer value of 18
				-- Write inverse value of dst1 (0->1, 1->0)
				-- to entry 2 in byte 4 (=18)
				Telegram_2.Value(Offset_Daylight_Saving_Time +
								1) := not DST1;
			elsif DST1 = No_Signal and DST2 /= No_Signal and
							DST2 /= No_Update then
				Telegram_2.Value(Offset_Daylight_Saving_Time
								) := not DST2;
			end if;
			return OK;
		end Daylight_Saving_Time;

		-- 20: entry has to match and be constant 1
		function Begin_Time return Inner_Checkresult is
		begin
			case Telegram_2.Value(Offset_Begin_Time) is
			when Bit_0 =>
				return Error_6; -- constant 1 violated
			when No_Signal =>
				-- unset => correct to 1
				Telegram_2.Value(Offset_Begin_Time) := Bit_1;
				return OK;
			when others =>
				return OK;
			end case;
		end Begin_Time;

		-- 59:entries have to match and be constant X
		-- (or special case leap second)
		-- new error results compared to C version.
		function End_Of_Minute return Inner_Checkresult is
			EOM1: constant Reading := Telegram_1.Value(
						Offset_Endmarker_Regular);
		begin
			if Telegram_2.Value(Offset_Endmarker_Regular) /=
								No_Signal then
				-- telegram 2 cannot be leap, must end on no
				-- signal
				return Error_7;
			elsif (Telegram_1_Is_Leap and EOM1 /= Bit_1) or
							(EOM1 = No_Signal) then
				return OK;
			else
				return Error_8;
			end if;
		end End_Of_Minute;

	begin
		return  Begin_Of_Minute                         = OK and then
			DST_Announce                            = OK and then
			Match(17, 20, Offset_Leap_Sec_Announce) = OK and then
			Daylight_Saving_Time                    = OK and then
			Begin_Time                              = OK and then
			Match(25, 58, Offset_Parity_Minute)     = OK and then
			End_Of_Minute                           = OK;
	end X_Eliminate;

	-- begin and end both incl
	function X_Eliminate_Match(Telegram_1: in Telegram;
			Telegram_2: in out Telegram;
			From, To, Except: in Natural) return Boolean is
	begin
		for I in From .. To loop
			if I /= Except and then not X_Eliminate_Entry(
					Telegram_1.Value(I),
					Telegram_2.Value(I)) then
				return False;
			end if;
		end loop;
		return True;
	end X_Eliminate_Match;

	function "not"(R: in Reading) return Reading is
	begin
		case R is
		when Bit_0  => return Bit_1;
		when Bit_1  => return Bit_0;
		when others => return R;
		end case;
	end "not";

	procedure X_Eliminate_Entry(TVI: in Reading; TVO: in out Reading) is
	begin
		if TVO = No_Signal or TVO = No_Update then
			TVO := TVI;
		end if;
	end X_Eliminate_Entry;

	function X_Eliminate_Entry(TVI: in Reading; TVO: in out Reading)
							return Boolean is
	begin
		if TVI = No_Signal or TVI = No_Update or TVI = TVO then
			-- no update
			return True; -- OK
		elsif TVO = No_Signal or TVO = No_Update then
			-- takes val 1
			TVO := TVI;
			return True;
		else
			-- mismatch
			return False;
		end if;
	end X_Eliminate_Entry;

	function BCD_Check_Minute(Minute_Ones_Raw, Minute_Tens_Raw: in Bits;
			Parity_Bit: in Reading) return Inner_Checkresult is
		Parity_Minute: Parity_State := Parity_Sum_Even_Pass;
		Minute_Ones: constant Natural := Decode_BCD(Minute_Ones_Raw,
								Parity_Minute);
		Minute_Tens: constant Natural := Decode_BCD(Minute_Tens_Raw,
								Parity_Minute);
	begin
		Update_Parity(Parity_Bit, Parity_Minute);
		-- 21-24 -- minute ones range from 0..9
		if Minute_Ones > 9 then
			return Error_4;
		-- 25-27 -- minute tens range from 0..5
		elsif Minute_Tens > 5 then
			return Error_5;
		-- telegram failing minute parity is invalid
		elsif Parity_Minute = Parity_Sum_Odd_Mismatch then
			return Error_6;
		else
			return OK;
		end if;
	end BCD_Check_Minute;

	procedure Update_Parity(Val: in Reading; Parity: in out Parity_State) is
	begin
		case Val is
		when Bit_1  => Parity := not Parity;
		when Bit_0  => null;
		when others => Parity := Parity_Sum_Undefined;
		end case;
	end Update_Parity;

	function "not"(Parity: in Parity_State) return Parity_State is
	begin
		case Parity is
		when Parity_Sum_Even_Pass    => return Parity_Sum_Odd_Mismatch;
		when Parity_Sum_Odd_Mismatch => return Parity_Sum_Even_Pass;
		when Parity_Sum_Undefined    => return Parity_Sum_Undefined;
		end case;
	end "not";

	-- When beginning to decode, supply Parity = Parity_Sum_Even_Pass
	-- When ready decoding, supply the last bit as parity update, then
	-- check if we are still at Parity_Sum_Even_Pass
	function Decode_BCD(Data: in Bits; Parity: in out Parity_State)
							return Natural is
		Mul:  Natural := 1;
		Rslt: Natural := 0;
	begin
		for Val of Data loop
			if Val = Bit_1 then
				Rslt := Rslt + Mul;
			end if;
			Mul := Mul * 2;
			Update_Parity(Val, Parity);
		end loop;
		return Rslt;
	end Decode_BCD;

end DCF77_ST_Layer_Shared;
