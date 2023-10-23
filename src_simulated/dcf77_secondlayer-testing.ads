package DCF77_Secondlayer.Testing is

	function X_Eliminate(Telegram_1_Is_Leap: in Boolean;
		Telegram_1: in DCF77_Secondlayer.Telegram;
		Telegram_2: in out DCF77_Secondlayer.Telegram) return Boolean;

	function Check_BCD_Correct_Telegram(
			Initial_Population: in DCF77_Secondlayer.Bits;
			Start_Offset_In_Line: in Natural) return Boolean;

end DCF77_Secondlayer.Testing;
