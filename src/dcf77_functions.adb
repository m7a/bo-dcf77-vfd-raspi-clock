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

	-- Parity-capable variant is part of DCF77_Secondlayer
	function Decode_BCD(Data: in Bits) return Natural is
		Mul:  Natural := 1;
		Rslt: Natural := 0;
	begin
		for Val of Data loop
			if Val = Bit_1 then
				Rslt := Rslt + Mul;
			end if;
			Mul := Mul * 2;
		end loop;
		return Rslt;
	end Decode_BCD;

end DCF77_Functions;
