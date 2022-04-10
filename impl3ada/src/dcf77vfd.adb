with DCF77_Low_Level;
with DCF77_Display;

procedure DCF77VFD is
	LL:   aliased DCF77_Low_Level.LL;
	Disp: aliased DCF77_Display.Disp;
begin
	LL.Init;
	Disp.Init(LL'Access);
end DCF77VFD;
