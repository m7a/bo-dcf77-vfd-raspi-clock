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
		Ctx.Inmode               := In_Backward;
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
		when In_Backward => Ctx.In_Backward(
						Val, Telegram_1, Telegram_2);
		when In_Forward  => Ctx.In_Forward(Val, Telegram_1, Telegram_2);
		end case;
	end Automaton_Case_Specific_Handling;

	procedure In_Backward(Ctx: in out Secondlayer; Val: in Reading;
			Telegram_1, Telegram_2: in out Telegram) is
		Current_Line_Is_Full: constant Boolean :=
			Ctx.Lines(Ctx.Line_Current).Value(0) /= No_Update;
	begin
		-- Cursor fixed at last bit index
		Ctx.Lines(Ctx.Line_Current).Value(Sec_Per_Min - 1) := Val;

		-- assert cursor > 0
		Ctx.Line_Cursor := Ctx.Line_Cursor - 1;

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
					Ctx.Lines(Ctx.Line_Current).Value(
						Ctx.Line_Cursor) := No_Signal;
					exit when Ctx.Line_Cursor = 0;
					Ctx.Line_Cursor := Ctx.Line_Cursor - 1;
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
			Ctx.Shift_Existing_Bits_To_The_Left;
		end if;
	end In_Backward;

	-- TODO CONTINUE TRANSLATION OF IMPLEMENTATION HERE
	procedure Process_Telegrams(Ctx: in out Secondlayer;
			Telegram_1, Telegram_2: in out Telegram) is null;

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

	-- TODO dcf77_secondlayer_moventries.c
	procedure Move_Entries_Backwards(Ctx: in out Secondlayer;
						Mov: in Natural) is null;

	function Check_BCD_Correct_Telegram(Ctx: in out Secondlayer;
			Start_Line: in Line_Num;
			Start_Offset_In_Line: in Natural) return Boolean is
		Next_Line:     constant Line_Num := Start_Line + 1;
		Is_Next_Empty: constant Boolean :=
					Ctx.Lines(Next_Line).Valid = Invalid;

		function Check_Begin_Of_Minute return Inner_Checkresult is
		begin
			return (Error_1); -- TODO CSTAT ...
		end Check_Begin_Of_Minute;

		function Check_DST return Inner_Checkresult is (Error_1);
		function Check_Begin_Of_Time return Inner_Checkresult is (Error_1);
		function Check_Minute return Inner_Checkresult is (Error_1);
		function Check_Hour return Inner_Checkresult is (Error_1);
		function Check_Date return Inner_Checkresult is (Error_1);

		function Check_Ignore_EOM_Inner return Boolean is
					(Check_Begin_Of_Minute = OK and then
					 Check_DST             = OK and then
					 Check_Begin_Of_Time   = OK and then
					 Check_Minute          = OK and then
					 Check_Hour            = OK and then
					 Check_Date            = OK);

		function Check_End_Of_Minute return Inner_Checkresult is
		begin
			return (if Is_Next_Empty then Error_1 else Error_1); -- TODO ...
		end Check_End_Of_Minute;
	begin
		return Check_Ignore_EOM_Inner and then Check_End_Of_Minute = OK;
	end Check_BCD_Correct_Telegram;

	procedure Process_Telegrams_Advance_To_Next_Line(
						Ctx: in out Secondlayer) is
	begin
		Ctx.Line_Cursor := 0;
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

end DCF77_Secondlayer;
