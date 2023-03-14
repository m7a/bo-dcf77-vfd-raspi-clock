package DCF77_Offsets is

	Sec_Per_Min: constant Natural := 60;
	--Sec_Per_Min_Leap: constant Natural := Sec_Per_Min + 1;

	Offset_Begin_Of_Minute:      constant Natural := 0;
	Offset_DST_Announce:         constant Natural := 16;
	Offset_Daylight_Saving_Time: constant Natural := 17; -- 17--18
	Offset_Leap_Sec_Announce:    constant Natural := 19;
	Offset_Begin_Time:           constant Natural := 20;
	Offset_Endmarker_Regular:    constant Natural := 59;

end DCF77_Offsets;
