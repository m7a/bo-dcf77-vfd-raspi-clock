with DCF77_Low_Level;
use  DCF77_Low_Level;

package DCF77_Ticker is

	Delay_Us_Target:   constant Time := 100_000;
	Delay_Us_Variance: constant Time := 10_000;
	Delay_Slowdown:    constant Time := 3;

	type Ticker is tagged limited private;

	procedure Init(Ctx: in out Ticker; LL: access DCF77_Low_Level.LL);
	procedure Tick(Ctx: in out Ticker);

private

	type Ticker is tagged limited record
		LL:       access DCF77_Low_Level.LL;
		Delay_Us: Time := Delay_Us_Target;
		Time_Old: Time;
	end record;

end DCF77_Ticker;
