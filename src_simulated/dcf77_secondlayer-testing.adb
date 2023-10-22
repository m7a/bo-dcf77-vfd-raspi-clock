package body DCF77_Secondlayer.Testing is

	function X_Eliminate(Telegram_1_Is_Leap: in Boolean;
		Telegram_1: in DCF77_Secondlayer.Telegram;
		Telegram_2: in out DCF77_Secondlayer.Telegram) return Boolean is
			(DCF77_Secondlayer.X_Eliminate(Telegram_1_Is_Leap,
			Telegram_1, Telegram_2));

end DCF77_Secondlayer.Testing;
