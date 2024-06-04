with Ada.Strings.Bounded;
with Ada.Containers.Indefinite_Vectors;

with Sax.Readers;
with Sax.Attributes;
with Unicode.CES;

with DCF77_Types;
with DCF77_SM_Layer_Shared;
with DCF77_TM_Layer_Shared;
with DCF77_Minutelayer;

package DCF77_Test_Data is

	package ST is new Ada.Strings.Bounded.Generic_Bounded_Length(Max => 128);
	Empty: ST.Bounded_String renames ST.Null_Bounded_String;

	type Uses is (None, X_Eliminate, Secondlayer, Check_BCD, Test_QOS,
			Unit);

	type Tel is record
		Len: Natural := 60;
		Val: DCF77_Types.Bits(0 .. 60) := (others =>
							DCF77_Types.No_Update);
	end record;

	type Checkpoint is record
		Loc: Natural;
		Val: DCF77_TM_Layer_Shared.TM;
		Q:   DCF77_Minutelayer.QOS;
	end record;

	type Tel_Array        is array (Natural range <>) of Tel;
	type Checkpoint_Array is array (Natural range <>) of Checkpoint;
	-- Usage |-> Description or empty if not assigned to this usage
	type Use_Array        is array (Uses) of ST.Bounded_String;

	type Spec(Num_In, Num_Out, Num_Checkpoints: Natural) is record
		Descr:              ST.Bounded_String;
		Use_For:            Use_Array;
		Input:              Tel_Array(1 .. Num_In);
		Input_Offset:       Natural;
		Output:             Tel_Array(1 .. Num_Out);
		Output_Recovery_OK: Boolean;
		Output_Faults:      Natural;
		Checkpoints:        Checkpoint_Array(1 .. Num_Checkpoints);
	end record;

	function Parse(XML_File: in String) return Spec;
	function XML_To_Use(S: in String) return Uses;

	function String_To_Tel(Val: in String) return Tel;
	function Length_To_Validity(Len: in Natural)
				return DCF77_SM_Layer_Shared.Telegram_State;
	function Tel_To_Telegram(Spt: in DCF77_Test_Data.Tel)
				return DCF77_SM_Layer_Shared.Telegram;

private

	package Tel_Vec is new Ada.Containers.Indefinite_Vectors(
			Index_Type => Natural, Element_Type => Tel);
	package Check_Vec is new Ada.Containers.Indefinite_Vectors(
			Index_Type => Natural, Element_Type => Checkpoint);

	type Tel_State is (Invalid, Input, Output);

	type Reader is new Sax.Readers.Reader with record
		State:              Tel_State := Invalid;
		Descr:              ST.Bounded_String := Empty;
		Use_For:            Use_Array := (others => Empty);
		Input:              Tel_Vec.Vector;
		Input_Offset:       Natural := 0;
		Output:             Tel_Vec.Vector;
		Output_Recovery_OK: Boolean := True;
		Output_Faults:      Natural := 0;
		Checkpoints:        Check_Vec.Vector;
	end record;

	function To_Array(Vec: in Tel_Vec.Vector) return Tel_Array;
	function To_Array(Vec: in Check_Vec.Vector) return Checkpoint_Array;

	procedure Start_Element(Handler: in out Reader;
			Namespace_URI: in Unicode.CES.Byte_Sequence := "";
			Lname:         in Unicode.CES.Byte_Sequence := "";
			Qname:         in Unicode.CES.Byte_Sequence := "";
			Atts:          in Sax.Attributes.Attributes'Class);
	function XML_To_Tel(Atts: in Sax.Attributes.Attributes'Class)
							return Tel;
	function XML_To_Checkpoint(Atts: in Sax.Attributes.Attributes'Class)
							return Checkpoint;
	procedure End_Element(Handler: in out Reader;
			Namespace_URI: in Unicode.CES.Byte_Sequence := "";
			Lname:         in Unicode.CES.Byte_Sequence := "";
			Qname:         in Unicode.CES.Byte_Sequence := "");

end DCF77_Test_Data;
