package body DCF77_Functions is

	procedure Inc_Saturated_Gen(Ctr: in out T; Lim: in T) is
	begin
		if Ctr < Lim then
			Ctr := Ctr + Step;
		end if;
	end Inc_Saturated_Gen;

	procedure Inc_Saturated_Natural is
				new Inc_Saturated_Gen(Natural, 1, "+", "<");
	procedure Inc_Saturated_Integer is
				new Inc_Saturated_Gen(Integer, 1, "+", "<");
	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural)
				renames Inc_Saturated_Natural;
	procedure Inc_Saturated_Int(Ctr: in out Integer; Lim: in Integer)
				renames Inc_Saturated_Integer;

	--function Num_To_Str(Num: in Natural; W: in Natural) return String is
	--	NC: Natural := Num;
	--	RV: String(1 .. W);
	--begin
	--	for I in reverse RV'Range loop
	--		RV(I) := Digit_Lut(NC mod 10);
	--		NC := NC / 10;
	--	end loop;
	--	return RV;
	--end Num_To_Str;

	function Num_To_Str_L4(Num: in Natural) return String is
			(Digit_Lut(Num / 1000), Digit_Lut((Num mod 1000) / 100),
			Digit_Lut((Num mod 100) / 10), Digit_Lut(Num mod 10));

	function Num_To_Str_L2(Num: in Natural) return String is
			(Digit_Lut(Num / 10), Digit_Lut(Num mod 10));

end DCF77_Functions;
