with DCF77_Types;
use  DCF77_Types;
with DCF77_ST_Layer_Shared;
use  DCF77_ST_Layer_Shared;

package DCF77_Timelayer is

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

	type TM is record
		Y: Natural; -- Year   0..9999
		M: Natural; -- Month  1..12
		D: Natural; -- Day    1..31
		H: Natural; -- Hour   0..23
		I: Natural; -- Minute 0..59
		S: Natural; -- Second 0..60 (60 = leapsec case)
	end record;

	type Timelayer is tagged limited private;

	-- TODO DETERMINE FROM COMPILE DATE AND WARN IF IMPOSSIBLE
	Time_Of_Compilation: constant TM := (2023, 11, 3, 20, 46, 47);

	procedure Init(Ctx: in out Timelayer);
	procedure Process(Ctx: in out Timelayer;
					Has_New_Bitlayer_Signal: in Boolean;
					Telegram_1, Telegram_2: in Telegram);
	function Get_Current(Ctx: in Timelayer) return TM;
	function Get_Quality_Of_Service(Ctx: in Timelayer) return QOS;

	-- Procedure is also useful for alarm implementation!
	procedure Advance_TM_By_Sec(T: in out TM; Seconds: in Natural);

-- Exported for testing --

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

	-- DST_Applied: If we have just switched betwen DST/regular then ignore
	-- announce bit for this very minute to not switch backwards again in
	-- the next minute.
	type DST_Switch is (DST_No_Change, DST_To_Summer, DST_To_Winter,
								DST_Applied);

	type Timelayer is tagged limited record
		-- Ring buffer of last minute ones bits.
		Preceding_Minute_Ones:  Minute_Buf;
		Preceding_Minute_Idx:   Minute_Buf_Idx;

		Prev:                   TM;
		Prev_Telegram:          DCF77_ST_Layer_Shared.Telegram;

		Seconds_Since_Prev:     Integer; -- -1 = unknown
		Seconds_Left_In_Minute: Natural; -- 0 = next minute

		Current:                TM;
		Current_QOS:            QOS;

		-- Denotes if at end of hour there will be a switch between
		-- summer and winter time.
		EOH_DST_Switch:         DST_Switch;
	end record;

	Month_Lengths: constant array (0 .. 12) of Integer := (
		29, --  0: leap year February
		31, --  1: January
		28, --  2: February
		31, --  3: March
		30, --  4: April
		31, --  5: May
		30, --  6: June
		31, --  7: July
		31, --  8: August
		30, --  9: September
		31, -- 10: October
		30, -- 11: November
		31  -- 12: December
	);

	type Recovery is (Data_Complete, Data_Incomplete_For_Minute,
						Data_Incomplete_For_Multiple);

	-- reference time point
	TM0: constant TM := Time_Of_Compilation;

	procedure Next_Minute_Coming(Ctx: in out Timelayer; Telegram_1,
						Telegram_2: in Telegram);
	function TM_To_Telegram_10min(T: in TM) return Telegram;
	procedure WMBC(TR: in out Telegram; Offset, Length, Value: in Natural);
	function Is_Leap_Year(Y: in Natural) return Boolean;
	procedure Process_New_Telegram(Ctx: in out Timelayer; Telegram_1_In,
						Telegram_2_In: in Telegram);
	function Recover_BCD(Tel: in out Telegram) return Recovery;
	procedure Drain_Assert(X: in Boolean);
	function Has_Minute_Tens(Tel: in Telegram) return Boolean;
	procedure Add_Minute_Ones_To_Buffer(Ctx: in out Timelayer;
							Tel: in Telegram);
	procedure Decode_And_Populate_DST_Switch(Ctx: in out Timelayer;
							Tel: in Telegram);
	function Decode_Tens(Tel: in Telegram) return TM;
	procedure Discard_Ones(T: in out TM);
	function Decode(Tel: in Telegram) return TM;
	function Decode_Check(Current: in out TM; Tel: in Telegram)
								return Boolean;
	function Recover_Ones(Ctx: in out Timelayer) return Integer;
	function Are_Ones_Compatible(AD, BD: in BCD_Digit) return Boolean;
	function Check_If_Compat_By_X_Eliminate(
				Virtual_Telegram: in out Telegram;
				Telegram_1: in Telegram) return Boolean;
	function TM_To_Telegram(T: in TM) return Telegram;
	function Try_QOS6(Ctx: in out Timelayer; Telegram_1: in Telegram)
							return Boolean;
	function Cross_Check_By_X_Eliminate(Ctx: in out Timelayer;
				Telegram_1: in out Telegram) return Boolean;

end DCF77_Timelayer;
