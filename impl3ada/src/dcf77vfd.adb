with DCF77_Low_Level;
with DCF77_Display;

procedure DCF77VFD is
	use type DCF77_Display.Pos_Y;
	LL:   aliased DCF77_Low_Level.LL;
	Disp: aliased DCF77_Display.Disp;
	Y0: DCF77_Display.Pos_Y := 16;
begin
	LL.Init;
	Disp.Init(LL'Access);

	LL.Log("BEFORE CTR=22");

	Y0 := 16;

	loop
		LL.Log("Hello");
		LL.Delay_Micros(500_000);
		Disp.Update((
			--1 => (X => 8, Y => 16, F => DCF77_DIsplay.Large, Msg =>
			--DCF77_Display.SB.To_Bounded_String("09:47:35")),
			1 => (X => 8, Y => Y0, F => DCF77_Display.Small, Msg =>
			DCF77_Display.SB.To_Bounded_String("Hello, World!"))
		));
		if Y0 = 16 then
			Y0 := 32;
		else
			Y0 := 16;
		end if;
	end loop;
end DCF77VFD;
