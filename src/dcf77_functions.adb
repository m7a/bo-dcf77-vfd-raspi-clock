package body DCF77_Functions is

	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural) is
	begin
		if Ctr < Lim then
			Ctr := Ctr + 1;
		end if;
	end Inc_Saturated;

	function Num_To_Str_L4(Num: in Natural) return String is
			(Digit_Lut(Num / 1000), Digit_Lut((Num mod 1000) / 100),
			Digit_Lut((Num mod 100) / 10), Digit_Lut(Num mod 10));

	function Num_To_Str_L2(Num: in Natural) return String is
			(Digit_Lut(Num / 10), Digit_Lut(Num mod 10));

end DCF77_Functions;
