with DCF77_Low_Level;
with DCF77_Display;
with DCF77_Ticker;

use DCF77_Low_Level; -- TODO z TMP Access time easily

procedure DCF77VFD is
	LL:     aliased DCF77_Low_Level.LL;
	Disp:   aliased DCF77_Display.Disp;
	Ticker: aliased DCF77_Ticker.Ticker;

	--use type DCF77_Display.Pos_Y;
	Y0: DCF77_Display.Pos_Y := 16;
begin
	LL.Init;
	Disp.Init(LL'Access);
	Ticker.Init(LL'Access);

	LL.Log("BEFORE CTR=23");

	Y0 := 16;
	loop
		LL.Log("Hello");
		Ticker.Tick;
		Disp.Update((
			--1 => (X => 8, Y => 16, F => DCF77_DIsplay.Large, Msg =>
			--DCF77_Display.SB.To_Bounded_String("09:47:35")),
			1 => (X => 8, Y => Y0, F => DCF77_Display.Small, Msg =>
			DCF77_Display.SB.To_Bounded_String("T" &
			Time'Image(LL.Get_Time_Micros)))
		));
		--if Y0 = 16 then
		--	Y0 := 32;
		--else
		--	Y0 := 16;
		--end if;
	end loop;

end DCF77VFD;
