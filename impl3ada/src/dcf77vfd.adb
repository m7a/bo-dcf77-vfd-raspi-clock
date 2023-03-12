with DCF77_Low_Level;
with DCF77_Display;
with DCF77_Ticker;
with DCF77_Bitlayer;
with DCF77_Secondlayer;

use DCF77_Low_Level; -- TODO z TMP Access time easily

procedure DCF77VFD is

	-- Hardware-specific
	LL:       aliased DCF77_Low_Level.LL;
	Disp:     aliased DCF77_Display.Disp;
	Ticker:   aliased DCF77_Ticker.Ticker;
	Bitlayer: aliased DCF77_Bitlayer.Bitlayer;

	Bitlayer_Reading: DCF77_Bitlayer.Reading;

	-- Software only
	Secondlayer:            aliased DCF77_Secondlayer.Secondlayer;
	Secondlayer_Telegram_1: DCF77_Secondlayer.Telegram;
	Secondlayer_Telegram_2: DCF77_Secondlayer.Telegram;

begin
	LL.Init;
	Disp.Init(LL'Access);
	Ticker.Init(LL'Access);
	Bitlayer.Init(LL'Access);
	Secondlayer.Init;

	LL.Log("BEFORE CTR=24");
	loop
		LL.Log("Hello");

		Bitlayer_Reading := Bitlayer.Update;
		Secondlayer.Process(Bitlayer_Reading,
				Secondlayer_Telegram_1, Secondlayer_Telegram_2);
		-- TODO ONWARDS WITH SECONDLAYER

		-- TODO X DEBUG ONLY
		Disp.Update((
			--1 => (X => 8, Y => 16, F => DCF77_DIsplay.Large, Msg =>
			--DCF77_Display.SB.To_Bounded_String("09:47:35")),
			1 => (X => 8, Y => 16, F => DCF77_Display.Small, Msg =>
			DCF77_Display.SB.To_Bounded_String("T" &
			Time'Image(LL.Get_Time_Micros))),
			2 => (X => 8, Y => 24, F => DCF77_Display.Small, Msg =>
			DCF77_Display.SB.To_Bounded_String(
			DCF77_Bitlayer.Reading'Image(Bitlayer_Reading)))
		));

		Ticker.Tick; -- last operation before next iteration
	end loop;

end DCF77VFD;
