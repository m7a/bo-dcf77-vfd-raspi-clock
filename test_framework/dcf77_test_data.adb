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
			Output             => To_Array(RD.Output),
			Output_Recovery_OK => RD.Output_Recovery_OK,
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
		elsif Lname = "checkpoint" then
			Check_Vec.Append(Handler.Checkpoints,
						XML_To_Checkpoint(Atts));
		else
			raise Constraint_Error with "Unknown element: <" &
								Lname & ">";
		end if;
	end Start_Element;

	function XML_To_Tel(Atts: in Sax.Attributes.Attributes'Class)
								return Tel is
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

		Val: constant Unicode.CES.Byte_Sequence :=
							Atts.Get_Value("val");
	begin
		return T: Tel do
			Assert(Val'Length <= 61);
			T.Len := Val'Length;
			for I in 0 .. T.Len - 1 loop
				T.Val(T.Val'First + I) :=
						Conv(Val(Val'First + I));
			end loop;
		end return;
	end XML_To_Tel;

	function XML_To_Checkpoint(Atts: in Sax.Attributes.Attributes'Class)
							return Checkpoint is
	begin
		return C: Checkpoint do
			C.Loc := Natural'Value(Atts.Get_Value("loc"));
			-- TODO x ASSIGN THE OTHER ATTRIBUTES AS SOON AS FMT IS KNOWN
		end return;
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

end DCF77_Test_Data;
