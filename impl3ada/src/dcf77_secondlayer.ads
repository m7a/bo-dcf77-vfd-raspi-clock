with DCF77_Bitlayer;
use  DCF77_Bitlayer; -- Reading
with DCF77_Offsets;
use  DCF77_Offsets;  -- Sec_Per_Min

package DCF77_Secondlayer is

	type Secondlayer    is tagged limited private;
	type Telegram_State is (Invalid, Valid_60, Valid_61);
	type Bits           is array (Natural range <>) of Reading;

	type Telegram is record
		Valid: Telegram_State := Invalid;
		Value: Bits(1 .. Sec_Per_Min) :=
					(others => DCF77_Bitlayer.No_Update);
	end record;
	
	procedure Init(Ctx: in out Secondlayer);
	procedure Process(Ctx: in out Secondlayer; Val: in Reading;
					Telegram_1, Telegram_2: out Telegram);

private

	Reset_Fault_Max: constant Natural := 1000;
	Lines:           constant Natural := 9;
	-- We make use of this value being the same as an "expired" timer... 
	Noleap:          constant Natural := 0;

	type Input_Mode is (
		In_Backward, -- Init mode. Push data backwards
		In_Forward   -- Aligned+Unknown mode. Push data forwards
	);

	type Telegram_Data is array (1 .. Lines) of Telegram;

	type Secondlayer is tagged limited record
		Inmode:       Input_Mode;
		Lines:        Telegram_Data;

		-- Current line to work on
		Line_Current: Natural;

		-- Position in line given in data points “bits”.
		-- Ranges from 1 to 60 both incl.
		--
		-- In forward mode, this sets the position to write the next bit
		-- to.
		--
		-- In backward mode, this sets the position to move the earliest
		-- bit to after writing the new bit to the end of the first
		-- line.
		Line_Cursor:  Natural;

		-- Gives the index of a line that has a leap second.	
		-- 
		-- The additional NO_SIGNAL for the leap second is not stored
		-- anywhere, but only reflected by this value. As long as there
		-- is no leap second recorded anywhere, this field has value
		-- Noleap (0).
		Leap_In_Line: Natural;

		-- Leap second announce timer/countdown in seconds.
		-- 
		-- Set to 70 * 60 seconds upon first seeing a leap second
		-- announce bit. This way, it will expire ten minutes after the
		-- latest point in time where the leap second could occur.
		Leap_Second_Expected: Natural;

		-- Number of resets performed
		Fault_Reset: Natural := 0;
	end record;

	procedure Reset(Ctx: in out Secondlayer);

	procedure Decrease_Leap_Second_Expectation(Ctx: in out Secondlayer);
	procedure Automaton_Case_Specific_Handling(Ctx: in out Secondlayer;
		Val: in Reading; Telegram_1, Telegram_2: in out Telegram);
	procedure In_Backward(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram);
	procedure Process_Telegrams(Ctx: in out Secondlayer;
				Telegram_1, Telegram_2: in out Telegram);
	procedure Shift_Existing_Bits_To_The_Left(Ctx: in out Secondlayer);
	procedure In_Forward(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram);

end DCF77_Secondlayer;
