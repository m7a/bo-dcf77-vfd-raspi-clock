package body DCF77_Display is

	procedure Init(Ctx: in out Disp; LL: access DCF77_Low_Level.LL) is
	begin
		Ctx.LL := LL;
	end Init;

end DCF77_Display;
