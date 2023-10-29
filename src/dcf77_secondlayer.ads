with DCF77_Types;
use  DCF77_Types;
with DCF77_Offsets;
use  DCF77_Offsets;  -- Sec_Per_Min

package DCF77_Secondlayer is

	type Secondlayer    is tagged limited private;
	type Telegram_State is (Invalid, Valid_60, Valid_61);
	type Bits           is array (Natural range <>) of Reading;

	type Telegram is record
		Valid: Telegram_State := Invalid;
		Value: Bits(0 .. Sec_Per_Min - 1) := (others => No_Update);
	end record;
	
	procedure Init(Ctx: in out Secondlayer);
	procedure Process(Ctx: in out Secondlayer; Val: in Reading;
					Telegram_1, Telegram_2: out Telegram);
	function Get_Fault(Ctx: in Secondlayer) return Natural;

private

	Reset_Fault_Max: constant Natural := 1000;
	Lines:           constant Natural := 9;
	-- We make use of this value being the same as an "expired" timer... 
	Noleap:          constant Natural := 0;

	type Input_Mode is (
		In_No_Signal, -- Only ever saw no signal / antenna is adjusting
		In_Backward,  -- First minute. Push data backwards
		In_Forward    -- Aligned+Unknown mode. Push data forwards
	);

	type Line_Num is mod Lines;
	type Telegram_Data is array (Line_Num) of Telegram;

	type Inner_Checkresult is (
		OK,        Error_1,  Error_2,   Error_3,  Error_4,  Error_5,
		Error_6,   Error_7,  Error_8,   Error_8b, Error_9,  Error_10,
		Error_10b, Error_11, Error_11b, Error_12, Error_13, Error_14,
		Error_15
	);

	type Parity_State is (Parity_Sum_Even_Pass, Parity_Sum_Odd_Mismatch,
							Parity_Sum_Undefined);

	type Secondlayer is tagged limited record
		Inmode:       Input_Mode;
		Lines:        Telegram_Data;

		-- Current line to work on
		Line_Current: Line_Num;

		-- Position in line given in data points “bits”.
		-- Ranges from 0 to 59 both incl.
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
		-- anywhere, but only reflected by this value.
		Has_Leap_In_Line: Boolean;
		Leap_In_Line:     Line_Num;

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
	procedure In_No_Signal(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram);
	procedure In_Backward(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram);
	procedure Process_Telegrams(Ctx: in out Secondlayer;
				Telegram_1, Telegram_2: in out Telegram);
	function X_Eliminate(Telegram_1_Is_Leap: in Boolean;
				Telegram_1: in Telegram;
				Telegram_2: in out Telegram) return Boolean;
	procedure X_Eliminate_Entry(TVI: in Reading; TVO: in out Reading);
	function X_Eliminate_Entry(TVI: in Reading; TVO: in out Reading)
							return Boolean;
	procedure Shift_Existing_Bits_To_The_Left(Ctx: in out Secondlayer);
	procedure In_Forward(Ctx: in out Secondlayer; Val: in Reading;
				Telegram_1, Telegram_2: in out Telegram);
	procedure Recompute_EOM(Ctx: in out Secondlayer);
	procedure Move_Entries_Backwards(Ctx: in out Secondlayer;
							Mov: in Natural);
	function Check_BCD_Correct_Telegram(Ctx: in out Secondlayer;
			Start_Line: in Line_Num;
			Start_Offset_In_Line: in Natural;
			Ignore_EOM: Boolean := False) return Boolean;
	function Decode_BCD(Data: in Bits; Parity: in out Parity_State)
							return Natural;
	procedure Update_Parity(Val: in Reading; Parity: in out Parity_State);
	function "not"(Parity: in Parity_State) return Parity_State;
	procedure Process_Telegrams_Advance_To_Next_Line(
						Ctx: in out Secondlayer);
	procedure Complex_Reorganization(Ctx: in out Secondlayer;
		Val: in Reading; Telegram_1, Telegram_2: in out Telegram);

end DCF77_Secondlayer;
