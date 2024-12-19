-- Shared info between Timelayer and Minutelayer
package DCF77_TM_Layer_Shared is

	type TM is record
		Y: Natural; -- Year   0..9999
		M: Natural; -- Month  1..12
		D: Natural; -- Day    1..31
		H: Natural; -- Hour   0..23
		I: Natural; -- Minute 0..59
		S: Natural; -- Second 0..60 (60 = leapsec case)
	end record;

	-- DST_Applied: If we have just switched betwen DST/regular then ignore
	-- announce bit for this very minute to not switch backwards again in
	-- the next minute.
	type DST_Switch is (DST_No_Change, DST_To_Summer, DST_To_Winter,
								DST_Applied);

	-- Cross-layer exchange data
	type TM_Exchange is record
		Is_New_Sec:   Boolean;
		Proposed:     TM;
		Is_Leapsec:   Boolean;
		Is_Confident: Boolean;
		DST_Delta_H:  Integer;
	end record;

	Min_Per_Hour:    constant Natural := 60;
	Hours_Per_Day:   constant Natural := 24;
	Months_Per_Year: constant Natural := 12;

	Time_Of_Compilation: constant TM := (2024, 12, 19, 23, 42, 08);

	-- public because it is of interest to GUI, too!
	Month_Lengths: constant array (0 .. 12) of Natural := (
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

	-- Function is also useful for GUI implementation!
	function Is_Leap_Year(Y: in Natural) return Boolean;
	function Get_Month_Length(T: in TM) return Natural;
	-- Procedure is also useful for alarm implementation!
	procedure Advance_TM_By_Sec(T: in out TM; Seconds: in Natural);

end DCF77_TM_Layer_Shared;
