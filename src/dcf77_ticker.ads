with DCF77_Low_Level;
use  DCF77_Low_Level;

package DCF77_Ticker is

	Delay_Us_Target:   constant Time := 100_000;
	Delay_Us_Variance: constant Time := 5_000;
	Delay_Slowdown:    constant Time := 3;

	type Ticker is tagged limited private;

	procedure Init(Ctx: in out Ticker; LL: in DCF77_Low_Level.LLP);
	procedure Tick(Ctx: in out Ticker);
	function Get_Delay(Ctx: in out Ticker) return Time;

private

	type Ticker is tagged limited record
		LL:       DCF77_Low_Level.LLP;
		Delay_Us: Time := Delay_Us_Target;
		Time_Old: Time;
	end record;

	function Get_Delay(Ctx: in out Ticker) return Time is (Ctx.Delay_Us);

end DCF77_Ticker;
