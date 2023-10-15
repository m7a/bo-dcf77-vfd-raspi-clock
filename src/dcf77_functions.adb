package body DCF77_Functions is

	procedure Inc_Saturated_Gen(Ctr: in out T; Lim: in T) is
	begin
		if Ctr < Lim then
			Ctr := Ctr + Step;
		end if;
	end Inc_Saturated_Gen;

	procedure Inc_Saturated_Natural is
				new Inc_Saturated_Gen(Natural, 1, "+", "<");
	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural)
				renames Inc_Saturated_Natural;

end DCF77_Functions;
