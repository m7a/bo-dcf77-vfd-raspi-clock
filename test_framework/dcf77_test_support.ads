with DCF77_Types;
with DCF77_TM_Layer_Shared;

package DCF77_Test_Support is

	procedure Invocation_Failed;
	procedure Test_Fail(Msg: in String);
	function Tel_Dump(B: in DCF77_Types.Bits) return String;
	procedure Test_Pass(Msg: in String);
	function TM_To_String(T: in DCF77_TM_Layer_Shared.TM) return String;

end DCF77_Test_Support;
