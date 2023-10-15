with Ada.Text_IO;
use  Ada.Text_IO;

with DCF77_Types;
use  DCF77_Types;
with DCF77_Secondlayer;
use  DCF77_Secondlayer;

procedure Test_Secondlayer is
	Testdata: constant String :=
		"222333330111100001001000101001100131100000111000011100010003" & -- 01.10.2023 13:28:00
		"000000110110100001001100101011100101100000111000011100013003" & -- 01.10.2023 13:29:00
		"011101110110101001001000011001100101100000111000011100010003" & -- 01.10.2023 13:30:00
		"001010100110100001001100011011100101100000111000011100010003" & -- 01.10.2023 13:31:00
		"000111111111001001001010011011100101100000111000011100010003" & -- 01.10.2023 13:32:00
		"001000101000010001001110011001100101100000111000011100010003" & -- 01.10.2023 13:33:00
		"0000100000010110010010010110";

	Ctx: Secondlayer;
	Use_Bit: Reading;

	Telegram_1: Telegram;
	Telegram_2: Telegram;

begin

	Ctx.Init;

	for I of Testdata loop
		case I is
		when '0' => Use_Bit := Bit_0;
		when '1' => Use_Bit := Bit_1;
		when '2' => Use_Bit := No_Update;
		when '3' => Use_Bit := No_Signal;
		when others => raise Constraint_Error with
						"Unknown symol '" & I & "'";
		end case;

		Put(I);
		Ctx.Process(Use_Bit, Telegram_1, Telegram_2);

		if Telegram_1.Valid = Valid_60 then
			New_Line;
			Put_Line(" => Valid telegram");
			Telegram_1.Valid := Invalid;
		end if;
	end loop;

end Test_Secondlayer;
