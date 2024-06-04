with Ada.Assertions;
use  Ada.Assertions;

with Input_Sources.File;
use  Input_Sources.File;
with Sax.Readers;
use  Sax.Readers;

-- XML interfacing inspired by https://docs.adacore.com/xmlada-docs/sax.html
package body DCF77_Test_Data is

	function Parse(XML_File: in String) return Spec is
		RD:    Reader;
		Input: File_Input;
	begin
		Open(XML_File, Input);
		RD.Set_Feature(Namespace_Prefixes_Feature, False);
		RD.Set_Feature(Namespace_Feature,          False);
		RD.Set_Feature(Validation_Feature,         False);
		RD.Parse(Input);
		Close(Input);
		return (
			Num_In             => Natural(RD.Input.Length),
			Num_Out            => Natural(RD.Output.Length),
			Num_Checkpoints    => Natural(RD.Checkpoints.Length),
			Descr              => RD.Descr,
			Use_For            => RD.Use_For,
			Input              => To_Array(RD.Input),
			Input_Offset       => RD.Input_Offset,
			Output             => To_Array(RD.Output),
			Output_Recovery_OK => RD.Output_Recovery_OK,
			Output_Faults      => RD.Output_Faults,
			Checkpoints        => To_Array(RD.Checkpoints)
		);
	end Parse;

	function To_Array(Vec: in Tel_Vec.Vector) return Tel_Array is
		RV: Tel_Array(1 .. Natural(Vec.Length));
		I:  Natural        := RV'First;
		C:  Tel_Vec.Cursor := Vec.First;
	begin
		while I <= RV'Last loop
			RV(I) := Tel_Vec.Element(C);
			I     := I + 1;
			C     := Tel_Vec.Next(C);
		end loop;
		return RV;
	end To_Array;

	function To_Array(Vec: in Check_Vec.Vector) return Checkpoint_Array is
		RV: Checkpoint_Array(1 .. Natural(Vec.Length));
		I:  Natural          := RV'First;
		C:  Check_Vec.Cursor := Vec.First;
	begin
		while I <= RV'Last loop
			RV(I) := Check_Vec.Element(C);
			I     := I + 1;
			C     := Check_Vec.Next(C);
		end loop;
		return RV;
	end To_Array;

	function XML_To_Use(S: in String) return Uses is
	begin
		if S = "none" then
			return None;
		elsif S = "xeliminate" then
			return X_Eliminate;
		elsif S = "secondlayer" then
			return Secondlayer;
		elsif S = "checkbcd" then
			return Check_BCD;
		elsif S = "qos" then
			return Test_QOS;
		elsif S = "unit" then
			return Unit;
		else
			raise Constraint_Error with "Unsupported usage: """ &
								S & """.";
		end if;
	end XML_To_Use;

	procedure Start_Element(Handler: in out Reader;
			Namespace_URI: in Unicode.CES.Byte_Sequence := "";
			Lname:         in Unicode.CES.Byte_Sequence := "";
			Qname:         in Unicode.CES.Byte_Sequence := "";
			Atts:          in Sax.Attributes.Attributes'Class) is
	begin
		if Lname = "dcf77testdata" then
			if Atts.Get_Index("descr") >= 0 then
				Handler.Descr := ST.To_Bounded_String(
						Atts.Get_Value("descr"));
			end if;	
		elsif Lname = "usefor" then
			Handler.Use_For(XML_To_Use(Atts.Get_Value("test"))) :=
				ST.To_Bounded_String(Atts.Get_Value("descr"));
		elsif Lname = "input" then
			Assert(Handler.State = Invalid);
			Handler.State := Input;
			if Atts.Get_Index("offset") >= 0 then
				Handler.Input_Offset := Natural'Value(
						Atts.Get_Value("offset"));
			end if;
		elsif Lname = "tel" then
			case Handler.State is
			when Invalid => raise Assertion_Error with
				"Telegram (<tel>) must either appear inside " &
				"<input> or <output> element.";
			when Input => Tel_Vec.Append(Handler.Input,
							XML_To_Tel(Atts));
			when Output => Tel_Vec.Append(Handler.Output,
							XML_To_Tel(Atts));
			end case;
		elsif Lname = "output" then
			Assert(Handler.State = Invalid);
			Handler.State := Output;
			Handler.Output_Recovery_OK :=
				Atts.Get_Value_As_Boolean("recovery_ok");
			if Atts.Get_Index("fault_reset_num_allowed") >= 0 then
				Handler.Output_Faults := Natural'Value(
						Atts.Get_Value(
						"fault_reset_num_allowed"));
			end if;
		elsif Lname = "checkpoint" then
			Check_Vec.Append(Handler.Checkpoints,
						XML_To_Checkpoint(Atts));
		else
			raise Constraint_Error with "Unknown element: <" &
								Lname & ">";
		end if;
	end Start_Element;

	function XML_To_Tel(Atts: in Sax.Attributes.Attributes'Class)
			return Tel is (String_To_Tel(Atts.Get_Value("val")));

	function String_To_Tel(Val: in String) return Tel is
		function Conv(Chr: in Character) return DCF77_Types.Reading is
		begin
			case Chr is
			when '0'    => return DCF77_Types.Bit_0;
			when '1'    => return DCF77_Types.Bit_1;
			when '2'    => return DCF77_Types.No_Update;
			when '3'    => return DCF77_Types.No_Signal;
			when others => raise Constraint_Error with
					"Unsupported reading value: '" & Chr &
					"'. Must be one of {0, 1, 2, 3}.";
			end case;
		end Conv;
	begin
		return T: Tel do
			Assert(Val'Length <= 61);
			T.Len := Val'Length;
			for I in 0 .. T.Len - 1 loop
				T.Val(T.Val'First + I) :=
						Conv(Val(Val'First + I));
			end loop;
		end return;
	end String_To_Tel;

	function XML_To_Checkpoint(Atts: in Sax.Attributes.Attributes'Class)
							return Checkpoint is
		function String_To_TM(S: in String)
						return DCF77_TM_Layer_Shared.TM
						with Pre => S'Length = 19 is
			S0: constant Integer := S'First;
		begin
			-- 2023-11-05 17:31:22
			-- 0123456789abcdefXYZ
			--            bc = 11 .. 12
			--               ef = 14 .. 15
			--                  YZ = 17 .. 18
			return (Y => Natural'Value(S(S0 +  0 .. S0 + 3)),
				M => Natural'Value(S(S0 +  5 .. S0 + 6)),
				D => Natural'Value(S(S0 +  8 .. S0 + 9)),
				H => Natural'Value(S(S0 + 11 .. S0 + 12)),
				I => Natural'Value(S(S0 + 14 .. S0 + 15)),
				S => Natural'Value(S(S0 + 17 .. S0 + 18)));
		end String_To_TM;
	begin
		return (Loc => Natural'Value(Atts.Get_Value("loc")),
			Val => String_To_TM(Atts.Get_Value("val")),
			Q => DCF77_Minutelayer.QOS'Value(Atts.Get_Value("qos")));
	end XML_To_Checkpoint;

	procedure End_Element(Handler: in out Reader;
			Namespace_URI: in Unicode.CES.Byte_Sequence := "";
			Lname:         in Unicode.CES.Byte_Sequence := "";
			Qname:         in Unicode.CES.Byte_Sequence := "") is
	begin
		if Lname = "input" then
			Assert(Handler.State = Input);
			Handler.State := Invalid;
		elsif Lname = "output" then
			Assert(Handler.State = Output);
			Handler.State := Invalid;
		end if;
	end End_Element;

	function Length_To_Validity(Len: in Natural) return
					DCF77_SM_Layer_Shared.Telegram_State is
	begin
		case Len is
		when 60 => return DCF77_SM_Layer_Shared.Valid_60;
		when 61 => return DCF77_SM_Layer_Shared.Valid_61;
		-- when others => Invalid; -- not here, see Assert
		when others => raise Assertion_Error with
			"Length must be 60 or 61. Found " & Natural'Image(Len);
		end case;
	end Length_To_Validity;

	function Tel_To_Telegram(Spt: in DCF77_Test_Data.Tel)
				return DCF77_SM_Layer_Shared.Telegram is
				(Length_To_Validity(Spt.Len), Spt.Val(0 .. 59));

end DCF77_Test_Data;
