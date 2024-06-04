with DCF77_Types;
use  DCF77_Types;
with DCF77_Offsets;
use  DCF77_Offsets;

-- Shared Functions between Secondlayer and Minutelayer.
package DCF77_SM_Layer_Shared is

	type Telegram_State is (Invalid, Valid_60, Valid_61);
	type Telegram is record
		Valid: Telegram_State := Invalid;
		Value: Bits(0 .. Sec_Per_Min - 1) := (others => No_Update);
	end record;

	type Inner_Checkresult is (
		OK,        Error_1,  Error_2,   Error_3,  Error_4,  Error_5,
		Error_6,   Error_7,  Error_8,   Error_8b, Error_9,  Error_10,
		Error_10b, Error_11, Error_11b, Error_12, Error_13, Error_14,
		Error_15
	);

	type Parity_State is (Parity_Sum_Even_Pass, Parity_Sum_Odd_Mismatch,
							Parity_Sum_Undefined);

	function X_Eliminate(Telegram_1_Is_Leap: in Boolean;
				Telegram_1: in Telegram;
				Telegram_2: in out Telegram) return Boolean;
	-- Performs an unconditional xeliminate on a specified range (except for
	-- given index). This is useful to e.g. check minute ones using
	-- xelimiate
	function X_Eliminate_Match(Telegram_1: in Telegram;
				Telegram_2: in out Telegram;
				From, To, Except: in Natural) return Boolean;

	procedure X_Eliminate_Entry(TVI: in Reading; TVO: in out Reading);

	-- Checks the validity of a given minute part of a telegram.
	-- This is needed on secondlayer and timelayer!
	function BCD_Check_Minute(Minute_Ones_Raw, Minute_Tens_Raw: in Bits;
			Parity_Bit: in Reading) return Inner_Checkresult;

	procedure Update_Parity(Val: in Reading; Parity: in out Parity_State);
	function "not"(Parity: in Parity_State) return Parity_State;

	function "not"(R: in Reading) return Reading;
	function Decode_BCD(Data: in Bits; Parity: in out Parity_State)
								return Natural;

private

	function X_Eliminate_Entry(TVI: in Reading; TVO: in out Reading)
							return Boolean;

end DCF77_SM_Layer_Shared;
