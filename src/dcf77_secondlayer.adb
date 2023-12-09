with DCF77_Functions;
use  DCF77_Functions; -- Inc_Saturated

package body DCF77_Secondlayer is

	procedure Init(Ctx: in out Secondlayer) is
	begin
		Ctx.Reset;
		-- reset number of resets "the first is free"
		Ctx.Fault_Reset := 0;
	end Init;

	procedure Reset(Ctx: in out Secondlayer) is
	begin
		Ctx.Inmode               := In_No_Signal;
		-- Initialize the buffer to all NO_UPDATE/empty (00)
		Ctx.Lines                := (others => (others => <>));
		Ctx.Line_Current         := 0;
		Ctx.Line_Cursor          := Sec_Per_Min - 1;
		Ctx.Has_Leap_In_Line     := False;
		Ctx.Leap_Second_Expected := Noleap;
		Inc_Saturated(Ctx.Fault_Reset, Reset_Fault_Max);
	end Reset;

	procedure Process(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: out Telegram) is
	begin
		Telegram_1.Valid := Invalid;
		Telegram_2.Valid := Invalid;
		if Val /= No_Update then
			Ctx.Decrease_Leap_Second_Expectation;
			Ctx.Automaton_Case_Specific_Handling(Val,
							Telegram_1, Telegram_2);
		end if;
	end Process;

	procedure Decrease_Leap_Second_Expectation(Ctx: in out Secondlayer) is
	begin
		if Ctx.Leap_Second_Expected /= Noleap then
			Ctx.Leap_Second_Expected :=
						Ctx.Leap_Second_Expected - 1;
		end if;
	end Decrease_Leap_Second_Expectation;

	procedure Automaton_Case_Specific_Handling(Ctx: in out Secondlayer;
		Val: in Reading; Telegram_1, Telegram_2: in out Telegram) is
	begin
		case Ctx.Inmode is
		when In_No_Signal => Ctx.In_No_Signal(Val, Telegram_1,
								Telegram_2);
		when In_Backward  => Ctx.In_Backward(Val, Telegram_1,
								Telegram_2);
		when In_Forward   => Ctx.In_Forward(Val, Telegram_1,
								Telegram_2);
		end case;
	end Automaton_Case_Specific_Handling;

	procedure In_No_Signal(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram) is
	begin
		-- Align processing to antenna delivering signals
		if Val /= No_Signal then
			Ctx.Inmode := In_Backward;
			Ctx.In_Backward(Val, Telegram_1, Telegram_2);
		end if;
	end In_No_Signal;

	procedure In_Backward(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram) is
		Current_Line_Is_Full: constant Boolean :=
			Ctx.Lines(Ctx.Line_Current).Value(0) /= No_Update;
	begin
		-- Cursor fixed at last bit index
		Ctx.Lines(Ctx.Line_Current).Value(Sec_Per_Min - 1) := Val;

		if Val = No_Signal then
			-- No signal might indicate: end of minute.
			-- Start a new line and switch to aligned mode.
			if not Current_Line_Is_Full then
				-- General case: line ended early. In this case
				-- set all the leading NO_UPDATE/00-bits to
				-- NO_SIGNAL/01 in order to mark the line as
				-- non-empty. It is important to not only mark
				-- the first item as non-empty because moves
				-- may cut off that first item non-empty marker!
				--
				-- To do this, we go backwards from the cursor's
				-- current position until the very beginning (0)
				-- and write one bit each
				loop
					Ctx.Line_Cursor := Ctx.Line_Cursor - 1;
					Ctx.Lines(Ctx.Line_Current).Value(
						Ctx.Line_Cursor) := No_Signal;
					exit when Ctx.Line_Cursor = 0;
				end loop;
				-- Else: special case: The first telegram
				-- started with its first bit. Hence, we already
				-- have a full minute in our memory. This does
				-- not make as much difference as previously
				-- thought, see below.
			end if;

			-- Switch to forward mode
			-- Change of cursor position is up to process_telegrams
			Ctx.Inmode                        := In_Forward;
			Ctx.Line_Cursor                   := Sec_Per_Min;
			Ctx.Lines(Ctx.Line_Current).Valid := Valid_60;

			-- Always process telegrams. This mainly serves to
			-- validate the current partial telegram but additionaly
			-- outputs the partial data received so far.
			-- Whether the upper layer can do something sensible
			-- with a “very incomplete” telegram need not be thought
			-- of here yet.
			Ctx.Process_Telegrams(Telegram_1, Telegram_2);
			-- If nothing is output that is not an error.
			-- It just means the data was misaligned and now moved.
			-- Will perform another attempt to process the data
			-- after some more seconds have arrived.
		elsif Current_Line_Is_Full then
			-- We processed 59 bits before, this is the 60. without
			-- an end of minute. This is bad data. Discard
			-- everything and start anew. Note that this fault may
			-- occur if the clock is turned on just at the exact
			-- beginning of a minute with a leap second. The chances
			-- for this are quite low, so we can well say it is
			-- most likely a fault!
			Ctx.Reset;
		else
			-- Now that we have added our input, move bits and
			-- increase number of bits in line.
			Ctx.Line_Cursor := Ctx.Line_Cursor - 1;
			Ctx.Shift_Existing_Bits_To_The_Left;
		end if;
	end In_Backward;

	procedure Process_Telegrams(Ctx: in out Secondlayer;
				Telegram_1, Telegram_2: in out Telegram) is

		-- this is not a "proper" X-elimination but copies minute value
		-- bits, leap second announce bit and minute parity from the
		-- last telegram processed prior to outputting something. It
		-- allows the higher layers to receive and process these bits
		-- although they are not set by the normal xelimination.
		procedure Add_Missing_Bits(TO: in out Telegram;
							TI: in Telegram) is
		begin
			X_Eliminate_Entry(TI.Value(Offset_Leap_Sec_Announce),
					TO.Value(Offset_Leap_Sec_Announce));
			for I in Offset_Minute_Ones .. Offset_Minute_Ones +
						Length_Minute_Ones - 1 loop
				X_Eliminate_Entry(TI.Value(I), TO.Value(I));
			end loop;
			X_Eliminate_Entry(TI.Value(Offset_Parity_Minute),
						TO.Value(Offset_Parity_Minute));
		end Add_Missing_Bits;

		procedure Check_For_Leapsec_Announce is
		begin
			-- Only set if no counter in place yet
			if Telegram_1.Value(Offset_Leap_Sec_Announce) = Bit_1
					and Ctx.Leap_Second_Expected = Noleap
					then
				-- leap sec lies at most one hour in the future.
				-- we allow +10 minutes s.t. existing telegrams
				-- (w/ leap sec) do not become invalid until the
				-- whole telegram from the leap second has
				-- passed out of memory.
				Ctx.Leap_Second_Expected := 70 * 60;
			end if;
		end Check_For_Leapsec_Announce;

		procedure Advance_To_Next_Line is
		begin
			Ctx.Line_Cursor  := 0;
			Ctx.Line_Current := Ctx.Line_Current + 1;
			-- clear line by marking as invalid
			Ctx.Lines(Ctx.Line_Current).Valid := Invalid;
		end Advance_To_Next_Line;

		-- Merge result keeps state of Try_Merge.
		-- Any Try_Merge operation populates these values and overwrites
		-- the previous contents.
		Match:           Boolean;
		Is_Leap_In_Line: Boolean;
		Line_Prev:       Line_Num := Ctx.Line_Current;
		Line:            Line_Num := Ctx.Line_Current;

		procedure Try_Merge is
		begin
			Match           := True;
			Is_Leap_In_Line := False;
			loop
				if Ctx.Lines(Line).Valid /= Invalid then
					Line_Prev := Line;
				end if;
				Line := Line + 1;
				-- ignore empty lines
				if Ctx.Lines(Line).Valid /= Invalid then
					Is_Leap_In_Line := Ctx.Has_Leap_In_Line
						and Line = Ctx.Leap_In_Line;
					Match := X_Eliminate(Is_Leap_In_Line,
						Ctx.Lines(Line), Telegram_1);
				end if;
				exit when Line = Ctx.Line_Current or not Match;
			end loop;
		end Try_Merge;

		procedure Postprocess is
		begin
			Add_Missing_Bits(Telegram_1, Ctx.Lines(Line));
			Check_For_Leapsec_Announce;
		end Postprocess;

		-- No mismatch at all means: No minute tens change in buffer.
		-- E.g. 13:00, 13:01, 13:02, 13:03, 13:04, ... 13:08
		procedure No_Mismatch is
		begin
			if Line_Prev /= Ctx.Line_Current then
				Cross_Out_Areas_With_Change_To_Zero(
					Ctx.Lines(Line_Prev),
					Ctx.Lines(Ctx.Line_Current),
					Telegram_1);
			end if;
			Postprocess;
			Telegram_1.Valid := (if Is_Leap_In_Line then Valid_61
								else Valid_60);
			Telegram_2.Valid := Invalid;
			if not Is_Leap_In_Line then
				Advance_To_Next_Line;
			end if;
		end No_Mismatch;

		-- Single mismatch means: Minute tens change in buffer. This
		-- may at most happen once.
		-- E.g.: 13:05, 13:06, 13:07, 13:08, 13:09, 13:10, ... 13:13
		procedure Single_Mismatch is
		begin
			-- Second telegram now contains previous minute (the
			-- data from begin of buffer up until mismatch
			-- exclusive) generically write 60...
			Telegram_2.Valid := Valid_60;

			-- Clear mismatching out_telegram_1, otherwise the
			-- xeliminates in try_merge might fail.
			Telegram_1.Value := (others => No_Signal);

			-- Repeat and write to actual output line is now one
			-- before the line that failed and which we reprocess.
			Line := Line - 1;
			Try_Merge;
			if Match then
				-- no further mismatch. Data in the buffer is
				-- fully consistent. Can output this as truth.
				Telegram_1.Valid := (if Is_Leap_In_Line
						then Valid_61 else Valid_60);
				Postprocess;
				-- Do not advance cursor if we have a leap
				-- second because cursor will stay in same line
				-- and reach index 60 next second.
				if not Is_Leap_In_Line then
					Advance_To_Next_Line;
				end if;
			else
				-- We got another mismatch.
				-- The data is not consistent.
				Telegram_1.Valid := Invalid;
				Telegram_2.Valid := Invalid;
				Ctx.Recompute_EOM;
			end if;
		end Single_Mismatch;

		procedure Correct_Minute is
		begin
			-- first clear buffers to no signal
			Telegram_1.Value := (others => No_Signal);
			Telegram_2.Value := (others => No_Signal);

			-- then merge till mismatch
			Try_Merge;
			if Match then
				No_Mismatch;
			else
				Single_Mismatch;
			end if;
		end Correct_Minute;
	begin
		Telegram_1.Valid := Invalid;
		Telegram_2.Valid := Invalid;
		-- Must ignore EOM here to accept leap second case
		if Check_BCD_Correct_Telegram(Ctx, Ctx.Line_Current,
				Start_Offset_In_Line => 0, Ignore_EOM => True)
		then
			Correct_Minute;
		else
			-- received data is invalid. This requires us to perform
			-- a recompute_eom()
			Ctx.Recompute_EOM;
		end if;
	end Process_Telegrams;

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

		-- begin and end both incl
		function Match(From, To, Except: in Natural)
						return Inner_Checkresult is
		begin
			for I in From .. To loop
				if I /= Except and then not X_Eliminate_Entry(
						Telegram_1.Value(I),
						Telegram_2.Value(I)) then
					return Error_4;
				end if;
			end loop;
			return OK;
		end Match;

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
			Match(16, 20, Offset_Leap_Sec_Announce) = OK and then
			Daylight_Saving_Time                    = OK and then
			Begin_Time                              = OK and then
			Match(25, 58, Offset_Parity_Minute)     = OK and then
			End_Of_Minute                           = OK;
	end X_Eliminate;

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

	-- Special handling for No_Mismatch case: When no mismatch is detected
	-- we may have missed it due to many “unset” values in our input (bad
	-- signal strength so to say). It could then happeh that xeliminate has
	-- reconstructed wrong values from prior to the switch. So if the last
	-- two lines processed indicate that one of the date or time fields has
	-- switched from non-zero to logical zero (0 for time and year,
	-- 1 for month and day fields) then we must not output these fields from
	-- that point upwards until there is at least one field which
	-- definitely has not switched to logical zero (the first one where this
	-- is the case still must not be reported since even if its non-zero its
	-- represented value may be off by one). Since xeliminate does not
	-- distinguish this, this dedicated procedure cleans up the result from
	-- xeliminate by removing from the output telegram all of the affected
	-- fields and setting them to the unfiltered values from the input
	-- telegram (Ctx.Lines(Ctx.Line_Current)) instead.
	procedure Cross_Out_Areas_With_Change_To_Zero(From, To: in Telegram;
						Telegram_1: in out Telegram) is
		type Check_Part is record
			Offset: Natural;
			Length: Natural;
			L0_Lsb: Reading; -- L0 := Logic 0
		end record;

		-- avoid propagating No_Update / epsilon values
		procedure Cross_Out_Part(C: in Check_Part) is
		begin
			for I in C.Offset .. C.Offset + C.Length - 1 loop
				Telegram_1.Value(I) := (if (To.Value(I) = Bit_0
					or To.Value(I) = Bit_1) then To.Value(I)
					else No_Signal);
			end loop;
		end Cross_Out_Part;

		Check_Places: constant array (1 .. 10) of Check_Part := (
			(Offset_Minute_Ones, Length_Minute_Ones, Bit_0),
			(Offset_Minute_Tens, Length_Minute_Tens, Bit_0),
			(Offset_Hour_Ones,   Length_Hour_Ones,   Bit_0),
			(Offset_Hour_Tens,   Length_Hour_Tens,   Bit_0),
			(Offset_Day_Ones,    Length_Day_Ones,    Bit_1),
			(Offset_Day_Tens,    Length_Day_Tens,    Bit_0),
			(Offset_Month_Ones,  Length_Month_Ones,  Bit_1),
			(Offset_Month_Tens,  Length_Month_Tens,  Bit_0),
			(Offset_Year_Ones,   Length_Year_Ones,   Bit_0),
			(Offset_Year_Tens,   Length_Year_Tens,   Bit_0)
		);
		From_Is_L0:      Boolean;
		To_Pot_L0:       Boolean;
		Crossout_Active: Boolean := False;
	begin
		for C of Check_Places loop
			if Crossout_Active then
				Cross_Out_Part(C);
			end if;
			From_Is_L0 := From.Value(C.Offset) = C.L0_Lsb;
			To_Pot_L0  := To.Value(C.Offset)  /= not C.L0_Lsb;
			for I in C.Offset + 1 .. C.Offset + C.Length - 1 loop
				From_Is_L0 := From_Is_L0 and From.Value(I) =
									Bit_0;
				To_Pot_L0  := To_Pot_L0  and To.Value(I)
								/= Bit_1;
			end loop;
			if (not From_Is_L0) and To_Pot_L0 then
				Cross_Out_Part(C);
				Crossout_Active := True;
			elsif Crossout_Active then
				-- The crossout area must always be continuous
				exit;
			end if;
		end loop;
	end Cross_Out_Areas_With_Change_To_Zero;

	procedure Shift_Existing_Bits_To_The_Left(Ctx: in out Secondlayer) is
	begin
		for I in Ctx.Line_Cursor .. (Sec_Per_Min - 2) loop
			Ctx.Lines(Ctx.Line_Current).Value(I) :=
				Ctx.Lines(Ctx.Line_Current).Value(I + 1);
		end loop;
	end Shift_Existing_Bits_To_The_Left;

	procedure In_Forward(Ctx: in out Secondlayer; Val: in Reading;
			Telegram_1, Telegram_2: in out Telegram) is
	begin
		if Ctx.Line_Cursor < Sec_Per_Min then
			Ctx.Lines(Ctx.Line_Current).Value(Ctx.Line_Cursor)
									:= Val;
		end if;

		if Ctx.Line_Cursor = Sec_Per_Min - 1 then
			Ctx.Line_Cursor := Ctx.Line_Cursor + 1;
			Ctx.Lines(Ctx.Line_Current).Valid := Valid_60;

			-- Just wrote the 60. bit (at index 59).
			-- This means we are at the end of a minute.
			-- The current value should reflect this or
			-- it might possibly be a leap second (if announced).
			if Val = No_Signal then
				-- no signal in any case means this is our
				-- end-of-minute marker
				Ctx.Process_Telegrams(Telegram_1, Telegram_2);
			elsif Val = Bit_0 and Ctx.Leap_Second_Expected > 0
									then
				if Ctx.Has_Leap_In_Line then
					-- Leap second was already detected
					-- before. Another leap second within
					-- the same 10 minute interval means
					-- error. While we could somehow
					-- reorganize, there is little
					-- confidence that it would work.
					-- Hence force reset.
					Ctx.Reset;
				else
					-- No leap second recorded yet --
					-- this could be it
					Ctx.Leap_In_Line := Ctx.Line_Current;
					Ctx.Has_Leap_In_Line := True;
					
					-- In this special case, cursor position
					-- 60 becomes available. The actual
					-- Val will not be written, though.
					Ctx.Process_Telegrams(Telegram_1,
								Telegram_2);
				end if;
			else
				-- A signal was received but in this specific
				-- data arrangement, there should definitely
				-- have been a `NO_SIGNAL` at this place.
				-- We need to reorganize the datastructure
				-- to align to the "reality".
				Ctx.Recompute_EOM;
			end if;
		elsif Ctx.Line_Cursor = Sec_Per_Min then
			-- this is only allowed in the case of leap seconds.
			-- (no separate check here, but one could assert that
			-- the leap sec counter is > 0)
			if Val = No_Signal then
				Ctx.Lines(Ctx.Line_Current).Valid := Valid_61;
				-- OK; this telegram was already processed in
				-- the second before and our "past" assumption
				-- that it would be a leap second turned out to
				-- be right. Now no need to process the newly
				-- received bit, but need to advance cursor,
				-- hence invoke that procedure.
				Ctx.Process_Telegrams_Advance_To_Next_Line;
			else
				-- fails to identify as leap second. This means
				-- the assumption of ending on a leap second was
				-- wrong. Trigger `complex_reorganization`.
				Ctx.Complex_Reorganization(Val, Telegram_1,
								Telegram_2);
			end if;
		else
			-- If we are not near the end, just quietly append
			-- received bits, i.e. advance cursor (the rest of that
			-- is already implemented above).
			Ctx.Line_Cursor := Ctx.Line_Cursor + 1;
		end if;
	end In_Forward;

	-- Precondition: Current line is "full", but does not end on EOM.
	--
	-- Needs to
	-- (1) forward-identify a new EOM marker starting from the end of the
	--     current first entry [which effectively means current+1 w/ skip
	--     empty]
	-- (2) once identified, move all bits backwards the difference between
	--     the old EOM and the newly identified EOM. This way, some bits get
	--     shifted off at the first entry and thus the amount of data
	--     lessens. This should re-use existing work on leftward shifting.
	-- (3) hand back to process_telegrams: this is not as trivial as it may
	--     sound. The problem is: there is actually no new telegram until
	--     the new "current" minute has finished. Thus will usually end
	--     there w/o returning new telegram data.
	--
	-- This new implementation based on the one from 2020/11/01 does not
	-- handle leap seconds gracefully to reduce complexity and enhance
	-- robustness...
	procedure Recompute_EOM(Ctx: in out Secondlayer) is
		Telegram_Start_Offset_In_Line: Natural  := 1;
		Start_Line:           constant Line_Num := Ctx.Line_Current + 1;
		Line:                          Line_Num;
		All_Checked:                   Boolean  := False;
	begin
		if Ctx.Has_Leap_In_Line then
			-- Some time ago, some telegram was processed as leap
			-- second telegram. No way to recover from this except
			-- by complex mid-data-interposition of leap sec bit.
			-- For the rarity of this case, ignore this and perform
			-- a reset.
			Ctx.Reset;
			return;
		end if;

		while Telegram_Start_Offset_In_Line < Sec_Per_Min and
							not All_Checked loop
			All_Checked := True;
			Line        := Start_Line;
			while Line /= Ctx.Line_Current and All_Checked loop
				-- indentation exceeded
				if Ctx.Lines(Line).Valid /= Invalid and then
				not Ctx.Check_BCD_Correct_Telegram(
				Line, Telegram_Start_Offset_In_Line) then
					All_Checked := False;
				end if;
				Line := Line + 1;
			end loop;
			Telegram_Start_Offset_In_Line :=
					Telegram_Start_Offset_In_Line + 1;
		end loop;

		if All_Checked then
			-- Everyone needs to move telegram_start_offset_in_line
			-- steps to the left. This honors the length of lines
			-- and considers the case of a reduction of the total
			-- number of lines.
			Ctx.Move_Entries_Backwards(
					Telegram_Start_Offset_In_Line - 1);
		else
			-- no suitable position found, data corruption or
			-- advanced leap sec case. requires reset
			Ctx.Reset;
		end if;
	end Recompute_EOM;

	-- dcf77_secondlayer_moventries.c
	procedure Move_Entries_Backwards(Ctx: in out Secondlayer;
						Mov: in Natural) is
		Bytes_Proc:   Natural  := 0;
		Input_Line_0: Line_Num := Ctx.Line_Current + 1; -- il0
		Input_Line:   Line_Num; -- il
		Output_Line:  Line_Num; -- ol
		Copy_From_In: Natural;
		Pos_In_Line:  Natural; -- pil
		Pos_Out_Line: Natural := 0; -- pol
	begin
		-- start from the first line in buffer. This is the first line
		-- following from the current which is not empty.
		while Ctx.Lines(Input_Line_0).Valid = Invalid and
				Input_Line_0 /= Ctx.Line_Current loop
			Input_Line_0 := Input_Line_0 + 1;
		end loop;

		Input_Line  := Input_Line_0;
		Output_Line := Input_Line_0;
		loop
			-- skip empty input lines (except for the current which
			-- could be "invalid" despite not being empty)
			if Input_Line = Ctx.Line_Current then
				Copy_From_In := Ctx.Line_Cursor;
			elsif Ctx.Lines(Input_Line).Valid = Invalid then
				Copy_From_In := 0;
			else
				Copy_From_In := Sec_Per_Min;
			end if;
				
			Pos_In_Line := 0;
			while Pos_In_Line < Copy_From_In loop
				if Bytes_Proc >= Mov then
					Ctx.Lines(Output_Line).Value(
						Pos_Out_Line) := Ctx.Lines(
						Input_Line).Value(Pos_In_Line);
					if Pos_Out_Line = Sec_Per_Min - 1 then
						Pos_Out_Line := 0;
						Output_Line  := Output_Line + 1;
					else
						Pos_Out_Line :=
							Pos_Out_Line + 1;
					end if;
				end if;
				Bytes_Proc  := Bytes_Proc + 1;
				Pos_In_Line := Pos_In_Line + 1;
			end loop;

			Input_Line := Input_Line + 1;
			exit when Input_Line = Input_Line_0;
		end loop;

		if Ctx.Line_Cursor >= Mov then
			Ctx.Line_Cursor := Ctx.Line_Cursor - Mov;
		else
			Ctx.Lines(Ctx.Line_Current).Valid := Invalid;
			Ctx.Line_Current := Ctx.Line_Current - 1;
			Ctx.Line_Cursor  := (Sec_Per_Min -
						(Mov - Ctx.Line_Cursor));
		end if;
	end Move_Entries_Backwards;

	function Check_BCD_Correct_Telegram(Ctx: in out Secondlayer;
			Start_Line: in Line_Num;
			Start_Offset_In_Line: in Natural;
			Ignore_EOM: Boolean := False) return Boolean is
		Next_Line:     constant Line_Num := Start_Line + 1;
		Is_Next_Empty: constant Boolean :=
					Ctx.Lines(Next_Line).Valid = Invalid;

		function Get_Bit(Idx: in Natural) return Reading is
			(if Start_Offset_In_Line + Idx >= Sec_Per_Min
			then (if Is_Next_Empty
				then No_Signal
				else Ctx.Lines(Next_Line).Value(
					Start_Offset_In_Line + Idx -
					Sec_Per_Min))
			else Ctx.Lines(Start_Line).Value(Start_Offset_In_Line +
					Idx));

		function Check_Begin_Of_Minute return Inner_Checkresult is
			-- begin of minute is constant 0
			(if Get_Bit(Offset_Begin_Of_Minute) = Bit_1
							then Error_1 else OK);

		function Check_DST return Inner_Checkresult is
			AN1: constant Reading :=
				Get_Bit(Offset_Daylight_Saving_Time);
			AN2: constant Reading :=
				Get_Bit(Offset_Daylight_Saving_Time + 1);
		begin
			return (if (AN1 = Bit_0 and AN2 = Bit_0) or
			           (AN1 = Bit_1 and AN2 = Bit_1) then
					Error_2 else OK);
		end Check_DST;

		function Get_Bits(Offset, Length: in Natural) return Bits is
			RV: Bits(1 .. Length);
		begin
			for I in RV'Range loop
				RV(I) := Get_Bit(Offset + I - 1);
			end loop;
			return RV;
		end Get_Bits;

		function Check_Begin_Of_Time return Inner_Checkresult is
					(if Get_Bit(Offset_Begin_Time) = Bit_0
					then Error_3 else OK);

		function Check_Minute return Inner_Checkresult is
			Parity_Minute: Parity_State := Parity_Sum_Even_Pass;
			Minute_Ones: constant Natural := Decode_BCD(Get_Bits(
					Offset_Minute_Ones, Length_Minute_Ones),
					Parity_Minute);
			Minute_Tens: constant Natural := Decode_BCD(Get_Bits(
					Offset_Minute_Tens, Length_Minute_Tens),
					Parity_Minute);
		begin
			Update_Parity(Get_Bit(Offset_Parity_Minute),
								Parity_Minute);
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
		end Check_Minute;

		function Check_Hour return Inner_Checkresult is
			Parity_Hour: Parity_State := Parity_Sum_Even_Pass;
			Hour_Ones: constant Natural := Decode_BCD(
				Get_Bits(Offset_Hour_Ones, Length_Hour_Ones),
				Parity_Hour);
			Hour_Tens: constant Natural := Decode_BCD(
				Get_Bits(Offset_Hour_Tens, Length_Hour_Tens),
				Parity_Hour);
		begin
			-- 29-32 -- hour ones range from 0..9
			if Hour_Ones > 9 then
				return Error_7;
			-- 33-34 -- hour tens range from 0..2
			-- (anything but 11 = 3 is valid)
			elsif Hour_Tens > 2 then
				return Error_8;
			-- If hour tens is 2 then hour ones may at most be 3
			elsif Hour_Tens = 2 and Hour_Ones > 3 then
				return Error_8b;
			end if;

			Update_Parity(Get_Bit(Offset_Parity_Hour), Parity_Hour);
			return (if Parity_Hour = Parity_Sum_Odd_Mismatch
				then Error_9 else OK);
		end Check_Hour;

		function Check_Date_Day(Parity_Date: in out Parity_State)
						return Inner_Checkresult is
			Day_Ones_Bits: constant Bits := Get_Bits(
					Offset_Day_Ones, Length_Day_Ones);
			Day_Ones: constant Natural := Decode_BCD(Day_Ones_Bits,
								Parity_Date);
			Day_Tens_Bits: constant Bits := Get_Bits(
					Offset_Day_Tens, Length_Day_Tens);
			Day_Of_Week_Bits: constant Bits := Get_Bits(
					Offset_Day_Of_Week, Length_Day_Of_Week);
		begin
			-- 36-39 -- day ones ranges from 0..9
			if Day_Ones > 9 then
				return Error_10;
			-- 40-41 -- all day tens are valid (0..3)
			-- If day tens is 0 then day ones must not be 0
			elsif Day_Tens_Bits = (Bit_0, Bit_0) and Day_Ones_Bits =
					(Bit_0, Bit_0, Bit_0, Bit_0) then
				return Error_10b;
			-- 42-44 -- all day of week (1-7) are valid except "0"
			elsif Day_Of_Week_Bits = (Bit_0, Bit_0, Bit_0) then
				return Error_11;
			end if;
			for Bit of Day_Tens_Bits loop
				Update_Parity(Bit, Parity_Date);
			end loop;
			for Bit of Day_Of_Week_Bits loop
				Update_Parity(Bit, Parity_Date);
			end loop;
			return OK;
		end Check_Date_Day;

		function Check_Date_Month(Parity_Date: in out Parity_State)
						return Inner_Checkresult is
			-- 45-48 -- month ones (0..9) -- nothing to check until
			--          tens
			Month_Ones_Bits: constant Bits := Get_Bits(
					Offset_Month_Ones, Length_Month_Ones);
			Month_Ones: constant Natural := Decode_BCD(
					Month_Ones_Bits, Parity_Date);
			-- 49    -- month tens are all valid (0..1)
			--          nothing to check
			Month_Tens_Bit: constant Reading :=
						Get_Bit(Offset_Month_Tens);
		begin
			-- If month tens is 1 then month ones is at most 2 or
			-- If month tens is 0 then month ones must not be 0
			if (Month_Tens_Bit = Bit_1 and Month_Ones > 2) or
				(Month_Tens_Bit = Bit_0 and Month_Ones_Bits =
					(Bit_0, Bit_0, Bit_0, Bit_0)) then
				return Error_11b;
			end if;
			Update_Parity(Month_Tens_Bit, Parity_Date);
			return OK;
		end Check_Date_Month;

		function Check_Date_Year(Parity_Date: in out Parity_State)
						return Inner_Checkresult is
			Year_Ones: constant Natural := Decode_BCD(
				Get_Bits(Offset_Year_Ones, Length_Year_Ones),
				Parity_Date);
			Year_Tens: constant Natural := Decode_BCD(
				Get_Bits(Offset_Year_Tens, Length_Year_Tens),
				Parity_Date);
		begin
			-- 50-53 -- year ones are valid from 0..9
			if Year_Ones > 9 then
				return Error_12;
			end if;
			-- 54-57 -- year tens are valid from 0..9
			if Year_Tens > 9 then
				return Error_13;
			end if;
			-- Check parity of entire date
			Update_Parity(Get_Bit(Offset_Parity_Date), Parity_Date);
			return (if Parity_Date = Parity_Sum_Odd_Mismatch then
				Error_14 else OK);
		end Check_Date_Year;

		function Check_Date return Inner_Checkresult is
			Parity_Date: Parity_State := Parity_Sum_Even_Pass;
			RV: Inner_Checkresult := Check_Date_Day(Parity_Date);
		begin
			if RV = OK then
				RV := Check_Date_Month(Parity_Date);
				if RV = OK then
					RV := Check_Date_Year(Parity_Date);
				end if;
			end if;
			return RV;
		end Check_Date;

		function Check_Ignore_EOM_Inner return Boolean is
					(Check_Begin_Of_Minute = OK and then
					 Check_DST             = OK and then
					 Check_Begin_Of_Time   = OK and then
					 Check_Minute          = OK and then
					 Check_Hour            = OK and then
					 Check_Date            = OK);

		function Check_End_Of_Minute return Inner_Checkresult is
		begin
			case Get_Bit(Offset_Endmarker_Regular) is
			when Bit_0 | Bit_1 => return Error_15;
			when others        => return OK;
			end case;
		end Check_End_Of_Minute;
	begin
		return Check_Ignore_EOM_Inner and then (Ignore_EOM or else
						Check_End_Of_Minute = OK);
	end Check_BCD_Correct_Telegram;

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

	procedure Process_Telegrams_Advance_To_Next_Line(
						Ctx: in out Secondlayer) is
	begin
		Ctx.Line_Cursor  := 0;
		Ctx.Line_Current := Ctx.Line_Current + 1;
		-- clear line by setting valid to false
		Ctx.Lines(Ctx.Line_Current).Valid := Invalid;
	end Process_Telegrams_Advance_To_Next_Line;

	-- This case is known as "complex reorganization"
	--
	-- Condition: Would have expected an end-of-minute marker. We have a
	-- mismatch although a leap-second was announced for around this time.
	--
	-- Action: We rewrite the data to step back one position such that the
	-- case is equivalent to cursor = 59 and NO_SIGNAL expected. Then, we
	-- call recompute_eom() to fix up the situation so far. Unlike in the
	-- case of the actual cursor = 59 we still need to process that one
	-- misplaced bit. We do this by repeating part of the computation.
	procedure Complex_Reorganization(Ctx: in out Secondlayer;
		Val: in Reading; Telegram_1, Telegram_2: in out Telegram) is
	begin
		Ctx.Line_Cursor      := Ctx.Line_Cursor - 1;
		Ctx.Has_Leap_In_Line := False;

		Ctx.Recompute_EOM;

		-- Process misplaced bit.
		-- Does not decrease leap second expectation again.
		Ctx.Automaton_Case_Specific_Handling(Val, Telegram_1,
								Telegram_2);
	end Complex_Reorganization;

	function Get_Fault(Ctx: in Secondlayer) return Natural is
							(Ctx.Fault_Reset);

end DCF77_Secondlayer;
