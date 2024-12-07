with DCF77_Offsets;
use  DCF77_Offsets;

package body DCF77_TM_Layer_Shared is

	-- https://en.wikipedia.org/wiki/Leap_year
	function Is_Leap_Year(Y: in Natural) return Boolean is (((Y mod 4) = 0)
				and (((Y mod 100) /= 0) or ((Y mod 400) = 0)));

	-- In case of leap year, access index 0 to return length of 29
	-- days for Feburary in leap years.
	function Get_Month_Length(T: in TM) return Natural is
				(Month_Lengths(if (T.M = 2 and
					Is_Leap_Year(T.Y)) then 0 else T.M));

	-- Not leap-second aware for now
	-- assert seconds < 12000, otherwise may output incorrect results!
	-- Currently does not work with negative times (implement them for
	-- special DST case by directly changing the hours `i` field).
	procedure Advance_TM_By_Sec(T: in out TM; Seconds: in Natural) is
		ML: Natural; -- month length cache variable
	begin
		T.S := T.S + Seconds;
		if T.S >= Sec_Per_Min then
			T.I := T.I + T.S /   Sec_Per_Min;
			T.S :=       T.S mod Sec_Per_Min;
			if T.I >= Min_Per_Hour then
				T.H := T.H + T.I /   Min_Per_Hour;
				T.I :=       T.I mod Min_Per_Hour;
				if T.H >= Hours_Per_Day then
					T.D := T.D + T.H /   Hours_Per_Day;
					T.H :=       T.H mod Hours_Per_Day;
					ML  := Get_Month_Length(T);
					if T.D > ML then
						T.D := T.D - ML;
						T.M := T.M + 1;
						if T.M > Months_Per_Year then
							T.M := 1;
							T.Y := T.Y + 1;
						end if;
					end if;
				end if;
			end if;
		end if;
	end Advance_TM_By_Sec;

end DCF77_TM_Layer_Shared;
