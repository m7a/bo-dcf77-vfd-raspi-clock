with Ada.Command_Line;
with Ada.Streams.Stream_IO;
with Ada.Exceptions;
with Ada.Text_IO;

with GNAT.Exception_Actions;
with GNAT.Traceback.Symbolic;

with DCF77_Functions;
use  DCF77_Functions;

with DCF77_Types;
with DCF77_SM_Layer_Shared;
with DCF77_Secondlayer;
with DCF77_Secondlayer.Testing;
with DCF77_TM_Layer_Shared;
with DCF77_Minutelayer;
with DCF77_Timelayer;

-- https://blog.adacore.com/running-american-fuzzy-lop-on-your-ada-code
procedure Test_Fuzz_QOS9 is

	use type DCF77_TM_Layer_Shared.TM;

	-- from dcf77_test_support.adb
	function TM_To_String(T: in DCF77_TM_Layer_Shared.TM) return String is
			(Num_To_Str_L4(T.Y) & "-" & Num_To_Str_L2(T.M) &
			"-" & Num_To_Str_L2(T.D) & " " & Num_To_Str_L2(T.H) &
			":" & Num_To_Str_L2(T.I) & ":" & Num_To_Str_L2(T.S));

	DCF_Fuzz_Time_Error: exception;
	DCF_Fuzz_QOS_Error:  exception;

	Recovery_Vals: constant String :=
		"" &
		"000010110000110000101100000011000001100100011010011100010013" &
		"001110111110001000101010000011000001100100011010011100010013" &
		"001101100111010000101110000001000001100100011010011100010013" &
		"001010110000011000101001000011000001100100011010011100010013" &
		"000111001010110000101101000001000001100100011010011100010013" &
		"011001001111110000101011000001000001100100011010011100010013" &
		"000000000111111000101111000011000001100100011010011100010013" &
		"011010110100101000101000100011000001100100011010011100010013" &
		"011110110110101000101100100001000001100100011010011100010013" &
		"001010010011110000101000010011000001100100011010011100010013" &
		"011100001010010000101100010001000001100100011010011100010013" &
		"";
	Recovery_TM: constant DCF77_TM_Layer_Shared.TM := (2023, 12, 9, 1, 11, 0);

	T1, T2: DCF77_SM_Layer_Shared.Telegram := (others => <>);
	SL:     DCF77_Secondlayer.Secondlayer;
	ML:     DCF77_Minutelayer.Minutelayer;
	Exch:   DCF77_TM_Layer_Shared.TM_Exchange;
	TL:     DCF77_Timelayer.Timelayer;

	procedure Process_Value(Val: in DCF77_Types.Reading) is
	begin
		SL.Process(Val, T1, T2);
		ML.Process(True, T1, T2, Exch);
		TL.Process(Exch);
		Ada.Text_IO.Put_Line("  " & DCF77_Types.Reading'Image(Val) &
				"  " & TM_To_String(TL.Get_Current) &
				" @" & ML.Get_QOS_Sym & TL.Get_QOS_Sym);
	end Process_Value;

	FD:           Ada.Streams.Stream_IO.File_Type;
	Input_Buffer: Ada.Streams.Stream_Element_Array(1 .. 1200) :=
								(others => 0);
	LST:          Ada.Streams.Stream_Element_Offset := Input_Buffer'Last;

begin
	if Ada.Command_Line.Argument_Count = 1 then
		Ada.Streams.Stream_IO.Open(FD, Ada.Streams.Stream_IO.In_File,
						Ada.Command_Line.Argument(1));
		Ada.Streams.Stream_IO.Read(FD, Input_Buffer, LST, 1);
		Ada.Streams.Stream_IO.Close(FD);
	end if;

	SL.Init;
	ML.Init;
	TL.Init;

	Ada.Text_IO.Put_Line("in...");
	Ada.Text_IO.Flush;

	for I in Input_Buffer'First .. LST loop
		case Input_Buffer(I) is
		when 16#30# => Process_Value(DCF77_Types.Bit_0);
		when 16#31# => Process_Value(DCF77_Types.Bit_1);
		when 16#33# => Process_Value(DCF77_Types.No_Signal);
		when others => null;
		end case;
	end loop;

	Ada.Text_IO.Put_Line("recover...");
	Ada.Text_IO.Flush;

	for I of Recovery_Vals loop
		case I is
		when '0' => Process_Value(DCF77_Types.Bit_0);
		when '1' => Process_Value(DCF77_Types.Bit_1);
		when '3' => Process_Value(DCF77_Types.No_Signal);
		when others => null;
		end case;
	end loop;

	Ada.Text_IO.Put_Line("T1 = " & TM_To_String(TL.Get_Current));
	Ada.Text_IO.Put_Line("Q1 = " & ML.Get_QOS_Sym & TL.Get_QOS_Sym);
	Ada.Text_IO.Flush;

	if TL.Get_Current /= Recovery_TM then
		raise DCF_Fuzz_Time_Error;
	end if;
	if ML.Get_QOS_Sym /= '1' then
		raise DCF_Fuzz_QOS_Error;
	end if;

	Ada.Text_IO.Put_Line("passed");

exception

when Occurrence: others =>
	Ada.Text_IO.Put_Line("exception!");
	-- https://www.reddit.com/r/ada/comments/hnu4ij/getting_exception_stack_trace/
	Ada.Text_IO.Put_Line(Ada.Exceptions.Exception_Information(Occurrence));
	Ada.Text_IO.Put_Line(
			GNAT.Traceback.Symbolic.Symbolic_Traceback(Occurrence)); 
	Ada.Text_IO.Put_Line("state info");
	DCF77_Secondlayer.Testing.Debug_Dump_State(SL);
	Ada.Text_IO.Put_Line("end state info");
	Ada.Text_IO.Flush;
	GNAT.Exception_Actions.Core_Dump(Occurrence);

end Test_Fuzz_QOS9;
