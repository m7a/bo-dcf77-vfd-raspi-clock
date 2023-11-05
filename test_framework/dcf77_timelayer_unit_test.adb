with DCF77_Types;
use  DCF77_Types;
with DCF77_Secondlayer;
with DCF77_Timelayer;
use  DCF77_Timelayer;
with DCF77_Timelayer.Testing;
with DCF77_Functions;
use  DCF77_Functions;

with DCF77_Test_Data; -- String_To_Tel
with DCF77_Test_Support;
use  DCF77_Test_Support;

package body DCF77_Timelayer_Unit_Test is

	procedure Run is
	begin
		Test_Are_Ones_Compatible;
		Test_Is_Leap_Year;
		Test_Advance_TM_By_Sec;
		Test_Recover_Ones;
		Test_Decode;
		Test_Telegram_Identity;
	end Run;

	procedure Test_Are_Ones_Compatible is
		type TV is record
			AD: BCD_Digit;
			BD: BCD_Digit;
			RS: Boolean;
		end record;

		function BDD(D: in BCD_Digit) return String is
			(Reading'Image(D(0)) & " " & Reading'Image(D(1)) & " " &
			 Reading'Image(D(2)) & " " & Reading'Image(D(3)));
		function TVD(T: in TV) return String is
			(BDD(T.AD) & ", " & BDD(T.BD) & " = " &
			 Boolean'Image(T.RS));

		Test_Vector: constant array (0 .. 8) of TV := (
			-- some arbitrary values are equivalent
			-- 0
			((others => Bit_0), (others => Bit_0), True),
			-- 1
			((2 => Bit_1, others => Bit_0),
			 (2 => Bit_1, others => Bit_0), True),

			-- cases where bits should be respected
			-- 2 fails to recover
			((Bit_0, Bit_0, Bit_0, Bit_0),
			 (Bit_1, Bit_0, Bit_0, Bit_0), False),

			-- cases where some bits are unset
			-- 3 succeeds despite seeming to mismatch
			((Bit_0,     Bit_0, Bit_0, Bit_0),
			 (No_Signal, Bit_0, Bit_0, Bit_0), True),
			-- 4
			((Bit_0,     Bit_0, Bit_0, Bit_0),
			 (No_Update, Bit_0, Bit_0, Bit_0), True),
			-- 5
			((No_Update, Bit_0, Bit_0, Bit_0),
			 (Bit_1,     Bit_0, Bit_0, Bit_0), True),

			-- mixed cases
			-- 6
			((No_Update, No_Update, Bit_0, Bit_0),
			 (Bit_1,     Bit_0,     Bit_0, Bit_0), True),
			-- 7
			((No_Update, No_Update, Bit_1, Bit_0),
			 (Bit_1,     Bit_0,     Bit_0, Bit_0), False),
			-- 8
			((No_Update, No_Update, Bit_0, Bit_0),
			 (Bit_1,     Bit_0,     Bit_1, Bit_0), False)
		);

		Prefix: constant String := "ones compatible ";

		RS: Boolean;
	begin
		for I in Test_Vector'Range loop
			RS := DCF77_Timelayer.Testing.Are_Ones_Compatible(
					Test_Vector(I).AD, Test_Vector(I).BD);
			if RS = Test_Vector(I).RS then
				Test_Pass(Prefix & TVD(Test_Vector(I)));
			else
				Test_Fail(Prefix & TVD(Test_Vector(I)));
			end if;
		end loop;
	end Test_Are_Ones_Compatible;

	procedure Test_Is_Leap_Year is
		type TV is record
			Y: Natural;
			Is_Leap: Boolean;
		end record;
		function TVD(T: in TV) return String is
					(Natural'Image(T.Y) & (if T.Is_Leap
					then " is leap" else " is regular"));
		Test_Vector: constant array (0 .. 6) of TV := (
			(2000, True),
			(2020, True),
			(2024, True),
			(2028, True),
			(1900, False),
			(2010, False),
			(2005, False)
		);
		Prefix: constant String := "leap test ";
		RS: Boolean;
	begin
		for I in Test_Vector'Range loop
			RS := DCF77_Timelayer.Testing.Is_Leap_Year(
							Test_Vector(I).Y);
			if RS = Test_Vector(I).Is_Leap then
				Test_Pass(Prefix & TVD(Test_Vector(I)));
			else
				Test_Fail(Prefix & TVD(Test_Vector(I)));
			end if;
		end loop;
	end Test_Is_Leap_Year;

	procedure Test_Advance_TM_By_Sec is
		type TV is record
			Input_TM:  TM;
			Delta_Sec: Integer;
			Output_TM: TM;
		end record;

		function TVD(T: in TV) return String is
					(TM_To_String(T.Input_TM) & " +" &
					Integer'Image(T.Delta_Sec) & " = " &
					TM_To_String(T.Output_TM));

		-- Deviate from indentation to have them all in one line
		Test_Vector: constant array (1 .. 9) of TV := (
		-- test 1: identity
		((2021, 10, 23, 22, 28, 30),    0, (2021, 10, 23, 22, 28, 30)),
		-- test 2: trivial
		((2021, 10, 23, 22, 28, 30),   13, (2021, 10, 23, 22, 28, 43)),
		-- test 3: inc hours
		((2021, 10, 23, 19, 57, 13), 9189, (2021, 10, 23, 22, 30, 22)),
		-- test 4: daywrap 30
		((2021,  9, 30, 23, 59,  0),   60, (2021, 10,  1,  0,  0,  0)),
		-- test 5: daywrap 31
		((2021,  7, 31, 23, 59, 59),   11, (2021,  8,  1,  0,  0, 10)),
		-- test 6: yearwrap (1185=45+19*60)
		((2020, 12, 31, 23, 40, 15), 1185, (2021,  1,  1,  0,  0,  0)),
		-- test 7: daywrap nonleap
		((2021,  2, 28, 22,  0,  0), 7243, (2021,  3,  1,  0,  0, 43)),
		-- test 8: daywrap leap
		((2020,  2, 28, 22,  0,  0), 7243, (2020,  2, 29,  0,  0, 43)),
		-- test 9: daywrap inleap (2621=(60-17-1)*60+(60-49)+90)
		((2020,  2, 29, 23, 17, 49), 2621, (2020,  3,  1,  0,  1, 30))
		-- More tests are defined in the C world for cases that the
		-- procedure cannot currently process. If it were extended for
		-- e.g. negative deltas or more than 12000 sec, it might make
		-- sense to have a look there
		);
		-- End deviate from indentation --

		Prefix: constant String := "advance TM ";
		CMP: TM;
	begin
		for I in Test_Vector'Range loop
			CMP := Test_Vector(I).Input_TM;
			DCF77_Timelayer.Testing.Advance_TM_By_Sec(CMP,
						Test_Vector(I).Delta_Sec);
			if CMP = Test_Vector(I).Output_TM then
				Test_Pass(Prefix & TVD(Test_Vector(I)));
			else
				Test_Fail(Prefix & TVD(Test_Vector(I)) &
						" expected, but arrived at " &
						TM_To_String(CMP) & " instead");
			end if;
		end loop;
	end Test_Advance_TM_By_Sec;

	procedure Test_Recover_Ones is
		type TV is record
			Preceding_Minute_Idx:  Minute_Buf_Idx;
			Preceding_Minute_Ones: Minute_Buf;
			Expected_Result:       Integer;
		end record;
		BCD: Minute_Buf renames DCF77_Timelayer.BCD_Comparison_Sequence;
		Nothing: constant BCD_Digit := (others => No_Signal);
		Test_Vector: constant array (1 .. 17) of TV := (
			-- test 1-3: recover from all-complete BCDs
			 1 => (0, (BCD(0), BCD(1), BCD(2), BCD(3), BCD(4),
				   BCD(5), BCD(6), BCD(7), BCD(8), BCD(9)), 0),
			 2 => (1, (BCD(0), BCD(1), BCD(2), BCD(3), BCD(4),
				   BCD(5), BCD(6), BCD(7), BCD(8), BCD(9)), 1),
			 3 => (6, (BCD(0), BCD(1), BCD(2), BCD(3), BCD(4),
				   BCD(5), BCD(6), BCD(7), BCD(8), BCD(9)), 6),
			-- test 4: unable to recover if everything is "empty"
			 4 => (0, (others => Nothing), -1),
			-- test 5: unable to recover if inconsistent data
			 5 => (0, (BCD(0), BCD(0), others => Nothing), -1),
			-- t. 6,7,8: recovers if at least two BCDs are complete
			 6 => (0, (BCD(0), BCD(1), others => Nothing), 0),
			 7 => (0, (2 => BCD(2), 3 => BCD(3), others => Nothing),
			       0),
			 8 => (0, (1 => BCD(1), 8 => BCD(8), others => Nothing),
			       0),
			-- test 9: recovers with a single complete BCD
			 9 => (0, (8 => BCD(8), others => Nothing), 0),
			-- test 10: recovers 1 for offset 1/single complete BCD
			10 => (0, (7 => BCD(8), others => Nothing), 1),
			-- test 11: recovers 0 for offset 1 + 1 (single BCD)
			11 => (1, (9 => BCD(0), others => Nothing), 2),
			-- test 12: “sequence 0000 7x_ ____ will not recover”
			-- NB: This test seems to be specified counter to its
			--     intention/description. This is being ignored for
			--     now since the test case is valid and passes
			--     although not very “original”
			12 => (0, (0 => BCD(0), others => Nothing), 0),
			-- test 13: sequence 0000 7x_ 1___ is for 0..8
			13 => (0, (0 => BCD(0),
			       8 => (Bit_1, No_Signal, No_Signal, No_Signal),
			       others => Nothing), 0),
			-- test 14: sequence 0000 8x_ 1___ is for 0..9
			14 => (0, (0 => BCD(0),
			       9 => (Bit_1, No_Signal, No_Signal, No_Signal),
			       others => Nothing), 0),
			-- test 15: sequence 0000 7x_ 1___ 1___ is for 0..9
			15 => (0, (0 => BCD(0),
				8 => (Bit_1, No_Signal, No_Signal, No_Signal),
				9 => (Bit_1, No_Signal, No_Signal, No_Signal),
				others => Nothing), 0),
			-- test 16: sequence 010_ ___1 is for 4..5
			16 => (0, (
				0 => (Bit_0,     Bit_1,     Bit_0, No_Signal),
				1 => (No_Signal, No_Signal, No_Signal, Bit_1),
				others => Nothing), 4),
			-- test 17: sequence 0___ 1___ is for 7..8
			17 => (0, (
				0 => (Bit_0, No_Signal, No_Signal, No_Signal),
				1 => (Bit_1, No_Signal, No_Signal, No_Signal),
				others => Nothing), 7)
		);
		Prefix: constant String := "recover ones test";
		Result: Integer;
	begin
		for I in Test_Vector'Range loop
			Result := DCF77_Timelayer.Testing.Test_Recover_Ones(
					Test_Vector(I).Preceding_Minute_Ones,
					Test_Vector(I).Preceding_Minute_Idx);
			if Result = Test_Vector(I).Expected_Result then
				Test_Pass(Prefix & Integer'Image(I));
			else
				Test_Fail(Prefix & Integer'Image(I) &
					" -- expected " & Integer'Image(
					Test_Vector(I).Expected_Result) &
					", got " & Integer'Image(Result));
			end if;
		end loop;
	end Test_Recover_Ones;

	procedure Test_Decode is
		Test_Vector: constant array (1 .. 2) of Tel_Conv_TV := (
			-- test 1: synthetic
			1 => ("000000000000000001001001001000000000100010011100101000010003", (2021, 9, 11,  0, 24, 0)),
			-- test 2: real
			2 => ("000010001011000001001100000100100010010001100001001001100013", (2019, 4, 22, 22, 41, 0))
		);
		Prefix: constant String := "decode ";
		Result: TM;
	begin
		for I in Test_Vector'Range loop
			Result := DCF77_Timelayer.Testing.Decode(
						DCF77_Test_Data.Tel_To_Telegram(
						DCF77_Test_Data.String_To_Tel(
						Test_Vector(I).Tel)));
			if Result = Test_Vector(I).TIme then
				Test_Pass(Prefix & TM_To_String(
							Test_Vector(I).Time));
			else
				Test_Pass(Prefix & TM_To_String(
					Test_Vector(I).Time) &
					" expected -- got " &
					TM_To_String(Result) & " for input " &
					Test_Vector(I).Tel & " instead");
			end if;
		end loop;
	end Test_Decode;

	procedure Test_Telegram_Identity is
		use type DCF77_Secondlayer.Telegram;

		Test_Vector: constant array (1 .. 1) of Tel_Conv_TV := (
			1 => ("333333333333333333333100000030110103100001333010000100000033", (2002, 2, 21, 16, 1, 0))
		);
		Prefix: constant String := "encode/identity ";
		Result: DCF77_Secondlayer.Telegram;
		Expect: DCF77_Secondlayer.Telegram;
	begin
		for I in Test_Vector'Range loop
			Expect := DCF77_Test_Data.Tel_To_Telegram(
						DCF77_Test_Data.String_To_Tel(
						Test_Vector(I).Tel));
			Result := DCF77_Timelayer.Testing.TM_To_Telegram(
						Test_Vector(I).Time);
			if Result = Expect then
				Test_Pass(Prefix & TM_To_String(
							Test_Vector(I).Time));
			else
				Test_Fail(Prefix &
					TM_To_String(Test_Vector(I).Time) &
					" -- expected " & Test_Vector(I).Tel &
					", got " & Tel_Dump(Result.Value));
			end if;
		end loop;
	end Test_Telegram_Identity;

end DCF77_Timelayer_Unit_Test;
