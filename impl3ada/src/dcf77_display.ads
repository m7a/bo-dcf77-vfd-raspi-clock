with DCF77_Low_Level;

package DCF77_Display is

	type Disp is tagged limited private;

	procedure Init(Ctx: in out Disp; LL: access DCF77_Low_Level.LL);

private

	type Disp is tagged limited record
		LL: access DCF77_Low_Level.LL;
	end record;

end DCF77_Display;
