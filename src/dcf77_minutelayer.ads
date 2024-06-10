with DCF77_Types;
use  DCF77_Types;
with DCF77_SM_Layer_Shared;
use  DCF77_SM_Layer_Shared;
with DCF77_TM_Layer_Shared;
use  DCF77_TM_Layer_Shared;

package DCF77_Minutelayer is

	type QOS is (
		QOS1,       -- +1 -- perfectly synchronized
		QOS2,       -- +2 -- synchr. w/ minor disturbance
		QOS3,       -- +3 -- synchronized from prev data
		QOS4,       -- o4 -- recovered from prev
		QOS5,       -- o5 -- async telegram match
		QOS6,       -- o6 -- count from prev (fishy, see code!)
		QOS7,       -- -7 -- async might match -1
		QOS8,       -- -8 -- async might match +1
		QOS9_ASYNC  -- -9 -- async count from last
	);

	type Minutelayer is tagged limited private;

	procedure Init(Ctx: in out Minutelayer);
	procedure Process(Ctx: in out Minutelayer;
					Has_New_Bitlayer_Signal: in Boolean;
					Telegram_1, Telegram_2: in Telegram;
					Exch: out TM_Exchange);

	-- GUI interaction
	procedure Set_TM_By_User_Input(Ctx: in out Minutelayer; T: in TM);
	function Get_QOS_Sym(Ctx: in Minutelayer) return Character;
	function Get_QOS_Stats(Ctx: in Minutelayer) return String;

-- Visible for testing only --

	subtype BCD_Digit is Bits(0 .. 3);

	-- 10 = last minute buf len
	type Minute_Buf_Idx is mod 10;
	type Minute_Buf     is array (Minute_Buf_Idx) of BCD_Digit;

	BCD_Comparison_Sequence: Minute_Buf := (
		-- Remember that DCF77 is all be encoding!
		-- Internal                   -- Decimal - Binary
		(Bit_0, Bit_0, Bit_0, Bit_0), -- 0       - 0 0 0 0
		(Bit_1, Bit_0, Bit_0, Bit_0), -- 1       - 0 0 0 1
		(Bit_0, Bit_1, Bit_0, Bit_0), -- 2       - 0 0 1 0
		(Bit_1, Bit_1, Bit_0, Bit_0), -- 3       - 0 0 1 1
		(Bit_0, Bit_0, Bit_1, Bit_0), -- 4       - 0 1 0 0
		(Bit_1, Bit_0, Bit_1, Bit_0), -- 5       - 0 1 0 1
		(Bit_0, Bit_1, Bit_1, Bit_0), -- 6       - 0 1 1 0
		(Bit_1, Bit_1, Bit_1, Bit_0), -- 7       - 0 1 1 1
		(Bit_0, Bit_0, Bit_0, Bit_1), -- 8       - 1 0 0 0
		(Bit_1, Bit_0, Bit_0, Bit_1)  -- 9       - 1 0 0 1
	);

private

	Unknown:  constant Integer := -1;
	Prev_Max: constant Integer := 32767; -- historical value per int16_t

	Leap_Sec_Processed:       constant Integer := -2;
	No_Leap_Sec_Announced:    constant Integer := -1;
	Leap_Sec_Newly_Announced: constant Integer := 0;
	Leap_Sec_Time_Limit:      constant Integer := 3670;

	type Stat_Entry is mod 2**32;
	type QOS_Stats_Array is array(QOS) of Stat_Entry;

	type Minutelayer is tagged limited record
		-- Year Hundreds defaults to compile-time value but may be
		-- changed by user input. Clock does not care if it by itself
		-- arrives at YH crossing and may fall back to leading “20” such
		-- as long as nothing was entered by the user explicitly.
		YH:                     Natural;

		-- Ring buffer of last minute ones bits.
		Preceding_Minute_Ones:  Minute_Buf;
		Preceding_Minute_Idx:   Minute_Buf_Idx;

		Prev:                   TM;
		Prev_Telegram:          DCF77_SM_Layer_Shared.Telegram;

		Seconds_Since_Prev:     Integer; -- -1 = unknown
		Seconds_Left_In_Minute: Natural; -- 0 = next minute

		Current:                TM;
		Current_QOS:            QOS;

		-- Denotes if at end of hour there will be a switch between
		-- summer and winter time.
		EOH_DST_Switch:         DST_Switch;

		-- -1: not announced
		-- -2: processed
		-- >= 0 announced and n sec passed
		Leap_Sec_State:         Integer;

		QOS_Stats:              QOS_Stats_Array;
	end record;

	type Recovery is (Data_Complete, Data_Incomplete_For_Minute,
						Data_Incomplete_For_Multiple);

	-- reference time point
	TM0: constant TM := Time_Of_Compilation;

	procedure Next_Minute_Coming(Ctx: in out Minutelayer; Telegram_1,
			Telegram_2: in Telegram; DST_Delta_H: in out Integer);
	function TM_To_Telegram_10min(T: in TM) return Telegram;
	procedure WMBC(TR: in out Telegram; Offset, Length, Value: in Natural);
	procedure Process_New_Telegram(Ctx: in out Minutelayer; Telegram_1_In,
						Telegram_2_In: in Telegram);
	function Recover_BCD(Tel: in out Telegram) return Recovery;
	procedure Drain_Assert(X: in Boolean);
	function Has_Minute_Tens(Tel: in Telegram) return Boolean;
	procedure Add_Minute_Ones_To_Buffer(Ctx: in out Minutelayer;
							Tel: in Telegram);
	procedure Decode_And_Populate_DST_Switch(Ctx: in out Minutelayer;
							Tel: in Telegram);
	function Decode_Tens(Ctx: in Minutelayer; Tel: in Telegram) return TM;
	procedure Discard_Ones(T: in out TM);
	function Decode(Ctx: in Minutelayer; Tel: in Telegram) return TM;
	function Decode_Check(Ctx: in out Minutelayer; Tel: in Telegram)
							return Boolean;
	function Recover_Ones(Ctx: in out Minutelayer) return Integer;
	function Are_Ones_Compatible(AD, BD: in BCD_Digit) return Boolean;
	function Check_If_Compat_By_X_Eliminate(
				Virtual_Telegram: in out Telegram;
				Telegram_1: in Telegram) return Boolean;
	function TM_To_Telegram(T: in TM) return Telegram;
	function Try_QOS6(Ctx: in out Minutelayer; Telegram_1: in Telegram)
							return Boolean;
	function Cross_Check_By_X_Eliminate(Ctx: in out Minutelayer;
				Telegram_1: in out Telegram) return Boolean;

end DCF77_Minutelayer;
