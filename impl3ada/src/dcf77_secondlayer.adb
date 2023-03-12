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
		Ctx.Line_Current         := Ctx.Lines'First;
		Ctx.Line_Cursor          := Sec_Per_Min;
		Ctx.Leap_In_Line         := Noleap;
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
			Ctx.Lines(Ctx.Line_Current).Value(1) /= No_Update;
	begin
		-- Cursor fixed at last bit index
		Ctx.Lines(Ctx.Line_Current).Value(Sec_Per_Min) := Val;

		-- assert cursor > 1
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
				-- current position until the very beginning (1)
				-- and write one bit each
				while Ctx.Line_Cursor > 0 loop
					Ctx.Lines(Ctx.Line_Current).Value(
						Ctx.Line_Cursor) := No_Signal;
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
			Ctx.Inmode      := In_Forward;
			Ctx.Line_Cursor := Sec_Per_Min;

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
		for I in Ctx.Line_Cursor .. (Sec_Per_Min - 1) loop
			Ctx.Lines(Ctx.Line_Current).Value(I) :=
				Ctx.Lines(Ctx.Line_Current).Value(I + 1);
		end loop;
	end Shift_Existing_Bits_To_The_Left;

	-- TODO TRANSLATION OF IMPLEMENTATION
	procedure In_Forward(Ctx: in out Secondlayer; Val: in Reading;
			Telegram_1, Telegram_2: in out Telegram) is null;

end DCF77_Secondlayer;
