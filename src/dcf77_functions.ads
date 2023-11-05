with DCF77_Types;
use  DCF77_Types;

package DCF77_Functions is

	procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural);

	procedure Inc_Saturated_Int(Ctr: in out Integer; Lim: in Integer);

	function Decode_BCD(Data: in Bits) return Natural;

	function Num_To_Str_L4(Num: in Natural) return String
						with Pre => (Num < 10000);
	function Num_To_Str_L2(Num: in Natural) return String
						with Pre => (Num < 100);

	-- TODO function may not be needed?
	--      the idea is that this one is generic but may be slower than
	--      L2/L4 variants.
	--function Num_To_Str(Num: in Natural; W: in Natural) return String;

private

	Digit_Lut: array(0 .. 9) of Character :=
			('0', '1', '2', '3', '4', '5', '6', '7', '8', '9');

	generic
		type T is private;
		Step: T;
		with function "+"(A, B: in T) return T;
		with function "<"(A, B: in T) return Boolean;
	procedure Inc_Saturated_Gen(Ctr: in out T; Lim: in T);
	
end DCF77_Functions;
