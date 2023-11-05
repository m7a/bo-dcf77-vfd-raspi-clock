with DCF77_Types;
with DCF77_Timelayer;

package DCF77_Test_Support is

	procedure Invocation_Failed;
	procedure Test_Fail(Msg: in String);
	function Tel_Dump(B: in DCF77_Types.Bits) return String;
	procedure Test_Pass(Msg: in String);
	function TM_To_String(T: in DCF77_Timelayer.TM) return String;

end DCF77_Test_Support;
