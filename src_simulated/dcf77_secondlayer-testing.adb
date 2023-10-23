package body DCF77_Secondlayer.Testing is

	function X_Eliminate(Telegram_1_Is_Leap: in Boolean;
		Telegram_1: in DCF77_Secondlayer.Telegram;
		Telegram_2: in out DCF77_Secondlayer.Telegram) return Boolean is
			(DCF77_Secondlayer.X_Eliminate(Telegram_1_Is_Leap,
			Telegram_1, Telegram_2));

	function Check_BCD_Correct_Telegram(
			Initial_Population: in DCF77_Secondlayer.Bits;
			Start_Offset_In_Line: in Natural) return Boolean is
		Ctx: Secondlayer;
		Line: Line_Num := 0;
		Pos_In_Line: Natural := 0;
	begin
		Ctx.Init;
		for I of Initial_Population loop
			Ctx.Lines(Line).Value(Pos_in_Line) := I;
			Ctx.Lines(Line).Valid              := Valid_60;
			Pos_In_Line := Pos_In_Line + 1;
			if Pos_In_Line = Sec_Per_Min then
				Pos_In_Line := 0;
				Line        := Line + 1;
			end if;
		end loop;
		return Check_BCD_Correct_Telegram(Ctx, 0, Start_Offset_In_Line);
	end Check_BCD_Correct_Telegram;

end DCF77_Secondlayer.Testing;
