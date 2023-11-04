with DCF77_Types;
use  DCF77_Types;

package DCF77_Functions is

	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural);

	procedure Inc_Saturated_Int(Ctr: in out Integer; Lim: in Integer);

	function Decode_BCD(Data: in Bits) return Natural;

private

	generic
		type T is private;
		Step: T;
		with function "+"(A, B: in T) return T;
		with function "<"(A, B: in T) return Boolean;
	procedure Inc_Saturated_Gen(Ctr: in out T; Lim: in T);
	
end DCF77_Functions;
