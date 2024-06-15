package DCF77_Functions is

	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural);

	function Num_To_Str_L4(Num: in Natural) return String
						with Pre => (Num < 10000);
	--function Num_To_Str_L3(Num: in Natural) return String
	--					with Pre => (Num < 1000);
	function Num_To_Str_L2(Num: in Natural) return String
						with Pre => (Num < 100);

private

	Digit_Lut: array(0 .. 9) of Character :=
			('0', '1', '2', '3', '4', '5', '6', '7', '8', '9');

end DCF77_Functions;
