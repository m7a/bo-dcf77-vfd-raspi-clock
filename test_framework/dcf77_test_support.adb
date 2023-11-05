with Ada.Command_Line;
with Ada.Text_IO;
use  Ada.Text_IO;

with DCF77_Functions;
use  DCF77_Functions;

package body DCF77_Test_Support is

	procedure Invocation_Failed is
	begin
		Ada.Command_Line.Set_Exit_Status(Ada.Command_Line.Failure);
	end Invocation_Failed;

	procedure Test_Fail(Msg: in String) is
	begin
		Put_Line("    [FAIL] " & Msg);
		Invocation_Failed;
	end Test_Fail;

	function Tel_Dump(B: in DCF77_Types.Bits) return String is
		RS: String(1 .. B'Length);
	begin
		for I in RS'Range loop
			case B(B'First + I - 1) is
			when DCF77_Types.Bit_0     => RS(I) := '0';
			when DCF77_Types.Bit_1     => RS(I) := '1';
			when DCF77_Types.No_Update => RS(I) := '2';
			when DCF77_Types.No_Signal => RS(I) := '3';
			end case;
		end loop;
		return RS;
	end Tel_Dump;

	procedure Test_Pass(Msg: in String) is
	begin
		Put_Line("    [ OK ] " & Msg);
	end Test_Pass;

	function TM_To_String(T: in DCF77_Timelayer.TM) return String is
			(Num_To_Str_L4(T.Y) & "-" & Num_To_Str_L2(T.M) &
			"-" & Num_To_Str_L2(T.D) & " " & Num_To_Str_L2(T.H) &
			":" & Num_To_Str_L2(T.I) & ":" & Num_To_Str_L2(T.S));

end DCF77_Test_Support;
