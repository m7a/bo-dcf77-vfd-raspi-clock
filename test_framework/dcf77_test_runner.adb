with Ada.Text_IO;
use  Ada.Text_IO;
with Ada.Assertions;
use  Ada.Assertions;
with Ada.Command_Line;
with Ada.Containers.Indefinite_Vectors;

with DCF77_Test_Data;

with DCF77_Offsets;
with DCF77_Types;
with DCF77_Secondlayer;
with DCF77_Secondlayer.Testing;

procedure DCF77_Test_Runner is

	----------------------------------------------[ Generic Test Support ]--

	use type DCF77_Test_Data.Spec;
	use type DCF77_Test_Data.Uses;
	use type DCF77_Test_Data.ST.Bounded_String;
	use type DCF77_Secondlayer.Bits;
	use type DCF77_Secondlayer.Telegram_State;

	procedure Invocation_Failed is
	begin
		Ada.Command_Line.Set_Exit_Status(Ada.Command_Line.Failure);
	end Invocation_Failed;

	procedure Test_Fail(Msg: in String) is
	begin
		Put_Line("    [FAIL] " & Msg);
		Invocation_Failed;
	end Test_Fail;

	function Tel_Dump(B: in DCF77_Secondlayer.Bits) return String is
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

	---------------------------------------------------------[ Test Uses ]--

	procedure Run_Check_BCD(Spec: in DCF77_Test_Data.Spec) is
		-- inefficient, but may work
		Empty_Bits: constant DCF77_Secondlayer.Bits(1 .. 0) :=
					(others => DCF77_Types.No_Update);
		function Flatten(B: in DCF77_Test_Data.Tel_Array)
						return DCF77_Secondlayer.Bits is
				(if B'Length = 0 then Empty_Bits else
				B(B'First).Val(0 .. B(B'First).Len - 1) &
				Flatten(B(B'First + 1 .. B'Last)));

		Prefix: constant String := DCF77_Test_Data.ST.To_String(
				Spec.Use_For(DCF77_Test_Data.Check_BCD));
		Got: constant Boolean :=
			DCF77_Secondlayer.Testing.Check_BCD_Correct_Telegram(
				Flatten(Spec.Input), Spec.Input_Offset);
	begin
		if Got = Spec.Output_Recovery_OK then
			Test_Pass(Prefix);
		else
			Test_Fail(Prefix &
				" -- checkbcd inverted result, expected=" &
				Boolean'Image(Spec.Output_Recovery_OK) &
				", got=" & Boolean'Image(Got));
		end if;
	end Run_Check_BCD;

	procedure Run_Secondlayer(Spec: in DCF77_Test_Data.Spec) is
		Cmp_Mask: constant array (0 .. DCF77_Offsets.Sec_Per_Min) of
			Integer :=
				(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				 1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,
				 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0);

		procedure Mask_Tel(Tel: in out DCF77_Secondlayer.Bits) is
		begin
			for I in Tel'Range loop
				if Cmp_Mask(I) = 0 then
					Tel(I) := DCF77_Types.No_Update;
				end if;
			end loop;
		end Mask_Tel;

		Prefix:   constant String := DCF77_Test_Data.ST.To_String(
				Spec.Use_For(DCF77_Test_Data.Secondlayer));
		Expect:   DCF77_Secondlayer.Bits :=
					Spec.Output(Spec.Output'Last).Val;
		Ctx:      DCF77_Secondlayer.Secondlayer;
		Cmp_Buf:  DCF77_Secondlayer.Telegram := (others => <>);
		Tel_1:    DCF77_Secondlayer.Telegram := (others => <>);
		Tel_2:    DCF77_Secondlayer.Telegram := (others => <>);

		--Seq:    Natural := 1;
	begin
		DCF77_Secondlayer.Init(Ctx);
		for Line of Spec.Input loop
			for I in  0 .. Line.Len - 1 loop
				DCF77_Secondlayer.Process(Ctx, Line.Val(I),
							Tel_1, Tel_2);
				--Ada.Text_IO.Put_Line("SEQ >" &
				--		Natural'Image(Seq) & "<");
				--Seq := Seq + 1;
				--DCF77_Secondlayer.Testing.Debug_Dump_State(Ctx);
				if Tel_1.Valid /= DCF77_Secondlayer.Invalid then
					Cmp_Buf := Tel_1;
					Tel_1   := (others => <>);
					Tel_2   := (others => <>);
				end if;
			end loop;
		end loop;
		-- now compare
		Mask_Tel(Expect);
		Mask_Tel(Cmp_Buf.Value);
		if Expect(0 .. DCF77_Offsets.Sec_Per_Min - 1) /=
							Cmp_Buf.Value then
			Test_Fail(Prefix & " -- expected=" & Tel_Dump(Expect) &
					", got=" & Tel_Dump(Cmp_Buf.Value));
		elsif DCF77_Secondlayer.Get_Fault(Ctx) /=
							Spec.Output_Faults then
			Test_Fail(Prefix & " -- mismatching number of faults" &
				", expected=" & Natural'Image(
				Spec.Output_Faults) & ", got=" & Natural'Image(
				DCF77_Secondlayer.Get_Fault(Ctx)));
		else
			Test_Pass(Prefix);
		end if;
	end Run_Secondlayer;

	procedure Run_X_Eliminate(Spec: in DCF77_Test_Data.Spec) is
		function Length_To_Validity(Len: in Natural) return
					DCF77_Secondlayer.Telegram_State is
		begin
			case Len is
			when 60 => return DCF77_Secondlayer.Valid_60;
			when 61 => return DCF77_Secondlayer.Valid_61;
			-- when others => Invalid; -- not here, see Assert
			when others => raise Assertion_Error with
					"Length must be 60 or 61. Found " &
					Natural'Image(Len);
			end case;
		end Length_To_Validity;

		function Spec_To_Tel(Spt: in DCF77_Test_Data.Tel)
					return DCF77_Secondlayer.Telegram is
			(Length_To_Validity(Spt.Len), Spt.Val(0 .. 59));

		Tel_Out: DCF77_Secondlayer.Telegram;
		Tel_In:  DCF77_Secondlayer.Telegram;
		Last_RS: Boolean := True;
		Prefix: constant String := DCF77_Test_Data.ST.To_String(
				Spec.Use_For(DCF77_Test_Data.X_Eliminate));
	begin
		Tel_In  := Spec_To_Tel(Spec.Input(1));
		for I in 2 .. Spec.Input'Length loop
			Tel_Out := Spec_To_Tel(Spec.Input(I));
			Last_RS := DCF77_Secondlayer.Testing.X_Eliminate
					(Tel_In.Valid =
					DCF77_Secondlayer.Valid_61, Tel_In,
					Tel_Out);
			Tel_In := Tel_Out;
		end loop;
		if Last_RS /= Spec.Output_Recovery_OK then
			Test_Fail(Prefix & " -- rv mismatch: expected=" &
					Boolean'Image(Spec.Output_Recovery_OK) &
					", got=" & Boolean'Image(Last_RS));
			return;
		elsif not Spec.Output_Recovery_OK then
			Test_Pass(Prefix);
		elsif Tel_Out.Value /= Spec.Output(1).Val(0 .. 59) then
			Test_Fail(Prefix & " -- output mismatch: expected=" &
					Tel_Dump(Spec.Output(1).Val(0 .. 59)) &
					", got=" & Tel_Dump(Tel_Out.Value));
		elsif Length_To_Validity(Spec.Output(1).Len) /= Tel_Out.Valid
									then
			Test_Fail(Prefix &
				" -- output validity mismatch: expected=" &
				Natural'Image(Spec.Output(1).Len) & ", got=" &
				DCF77_Secondlayer.Telegram_State'Image(
				Tel_Out.Valid));
		else
			Test_Pass(Prefix);
		end if;
	end Run_X_Eliminate;

	----------------------------------------------------[ Runner Details ]--

	package SV is new Ada.Containers.Indefinite_Vectors(
		Index_Type => Natural, Element_Type => DCF77_Test_Data.Spec);

	Specs:         SV.Vector;
	Continue_Run:  Boolean := True;
	Filter_Active: Boolean := False;
	Filter_Test:   DCF77_Test_Data.Uses := DCF77_Test_Data.None;

	procedure Help is
	begin
		Continue_Run := False;
		Put_Line("Ma_Sys.ma DCF77 VFD Raspi Clock Test Runner " &
				"(c) 2023 Ma_Sys.ma <info@masysma.net>");
		New_Line;
		Put_Line("USAGE " & Ada.Command_Line.Command_Name &
						" [-f FILTER] FILE.xml...");
		New_Line;
		Put_Line("-f FILTER  Run only one type of test. " &
						"Typical filter values:");
		Put_Line("           xeliminate - run x_eliminate test cases");
		New_Line;
		Put_Line("FILE.xml   Any number of XML test specs. " &
						"Typical invocation:");
		Put_Line("           ../test_data/*.xml");
		New_Line;
		Put_Line("EXAMPLE: " & Ada.Command_Line.Command_Name &
							" ../test_data/*.xml");
	end Help;

	procedure Parse_Arg(I: in out Positive) is
		Arg: constant String := Ada.Command_Line.Argument(I);
	begin
		Assert(Arg'Length > 1);
		if Arg(Arg'First) = '-' then
			case Arg(Arg'First + 1) is
			when 'h' | '-' | '?' =>
				Help;
			when 'f' =>
				Filter_Test := DCF77_Test_Data.XML_To_Use(
					Ada.Command_Line.Argument(I + 1));
				Filter_Active := True;
				I := I + 1;
			when others =>
				Specs.Append(DCF77_Test_Data.Parse(Arg));
			end case;
		else
			Specs.Append(DCF77_Test_Data.Parse(Arg));
		end if;
	end Parse_Arg;

	procedure Parse_Args is
		I: Positive := 1;
	begin
		while Continue_Run and I <= Ada.Command_Line.Argument_Count loop
			Parse_Arg(I);
			I := I + 1;
		end loop;
	end Parse_Args;

	procedure Run_Test_Of_Type(T: in DCF77_Test_Data.Uses;
					Spec: in DCF77_Test_Data.Spec) is
	begin
		case T is
		when DCF77_Test_Data.X_Eliminate => Run_X_Eliminate(Spec);
		when DCF77_Test_Data.Secondlayer => Run_Secondlayer(Spec);
		when DCF77_Test_Data.Check_BCD   => Run_Check_BCD(Spec);
		when others => raise Constraint_Error with
				"Cannot run test """ &
				DCF77_Test_Data.ST.To_String(Spec.Descr) &
				""" for " & DCF77_Test_Data.Uses'Image(T) & ".";
		end case;
	end Run_Test_Of_Type;

	procedure Run_Test_Of_Type(T: in DCF77_Test_Data.Uses) is
		I: Natural   := 1;
		C: SV.Cursor := Specs.First;
	begin
		-- "None" tests can only run under special procedures, not under
		-- the generic ones defined by this test framework.
		if T = DCF77_Test_Data.None then
			return;
		end if;
		Put_Line(DCF77_Test_Data.Uses'Image(T));
		while I <= Natural(Specs.Length) loop
			declare
				Spec: constant DCF77_Test_Data.Spec :=
								SV.Element(C);
			begin
				if Spec.Use_For(T) /= DCF77_Test_Data.Empty then
					Run_Test_Of_Type(T, Spec);
				end if;
			end;
			I := I + 1;
			C := SV.Next(C);
		end loop;
	end Run_Test_Of_Type;

begin

	Parse_Args;

	if not Continue_Run then
		Invocation_Failed;
		return;
	end if;

	if Filter_Active then
		Run_Test_Of_Type(Filter_Test);
	else
		for I in DCF77_Test_Data.Uses loop
			Run_Test_Of_Type(I);
		end loop;
	end if;

end DCF77_Test_Runner;