with DCF77_Secondlayer;

package DCF77_Timelayer is

	type QOS is (
		QOS1,       -- +1 -- perfectly synchronized
		QOS2,       -- +2 -- synchr. w/ minor disturbance
		QOS3,       -- +3 -- synchronized from prev data
		QOS4,       -- o4 -- recovered from prev
		QOS5,       -- o5 -- async telegram match
		QOS6,       -- o6 -- count from prev
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
			Telegram_1, Telegram_2: in DCF77_Secondlayer.Telegram);
	function Get_Current(Ctx: in Timelayer) return TM is (Ctx.Current);
	function Get_Quality_Of_Service(Ctx: in Timelayer) return QOS;

private

	Prev_Unknown: constant Integer := -1;
	Prev_Max:     constant Integer := 32767; -- historical value per int16_t

	-- DST_Applied: If we have just switched betwen DST/regular then ignore
	-- announce bit for this very minute to not switch backwards again in
	-- the next minute.
	type DST_Switch is (DST_No_Change, DST_To_Summer, DST_To_Winter,
								DST_Applied);

	-- 10 = last minute buf len
	type Minute_Buf_Idx is mod 10;
	type Minute_Buf: array (Minute_Buf_Idx) of
					DCF77_Secondlayer.Bits(0 .. 4);

	type Timelayer is record
		-- Ring buffer of last minute ones bits.
		Preceding_Minute_Ones:  Minute_Buf;
		Preceding_Minute_Idx:   Minute_Buf_Idx;

		Prev:                   TM;
		Prev_Telegram:          DCF77_Secondlayer.Telegram;

		Seconds_Since_Prev:     Integer; -- -1 = unknown
		Seconds_Left_In_Minute: Natural; -- 0 = next minute

		Current:                TM;
		Current_QOS:            QOS;

		-- Denotes if at end of hour there will be a switch between
		-- summer and winter time.
		EOH_DST_Switch:         DST_Switch;
	end record;

	Month_Lengths: constant Integer(0 .. 12) := (
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

end DCF77_Timelayer;
