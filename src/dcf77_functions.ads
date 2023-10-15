package DCF77_Functions is

	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural);

private

	generic
		type T is private;
		Step: T;
		with function "+"(A, B: in T) return T;
		with function "<"(A, B: in T) return Boolean;
	procedure Inc_Saturated_Gen(Ctr: in out T; Lim: in T);
	
end DCF77_Functions;
