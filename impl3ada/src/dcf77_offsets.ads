package DCF77_Offsets is

	Sec_Per_Min: constant Natural := 60;
	--Sec_Per_Min_Leap: constant Natural := Sec_Per_Min + 1;

	Offset_Begin_Of_Minute:      constant Natural := 0;
	Offset_DST_Announce:         constant Natural := 16;
	Offset_Daylight_Saving_Time: constant Natural := 17; -- 17--18
	Offset_Leap_Sec_Announce:    constant Natural := 19;
	Offset_Begin_Time:           constant Natural := 20;
	Offset_Minute_Ones:          constant Natural := 21;
	Offset_Minute_Tens:          constant Natural := 25; -- 25-27
	Offset_Parity_Minute:        constant Natural := 28;
	Offset_Hour_Ones:            constant Natural := 29;
	Offset_Hour_Tens:            constant Natural := 33;
	Offset_Parity_Hour:          constant Natural := 35;
	Offset_Day_Ones:             constant Natural := 36;
	Offset_Day_Tens:             constant Natural := 40;
	Offset_Day_Of_Week:          constant Natural := 42;
	Offset_Endmarker_Regular:    constant Natural := 59;

	Length_Minute_Ones:          constant Natural := 4;
	Length_Minute_Tens:          constant Natural := 3;
	Length_Hour_Ones:            constant Natural := 4;
	Length_Hour_Tens:            constant Natural := 2;
	Length_Day_Ones:             constant Natural := 4;
	Length_Day_Tens:             constant Natural := 2;
	Length_Day_Of_Week:          constant Natural := 3;

end DCF77_Offsets;
