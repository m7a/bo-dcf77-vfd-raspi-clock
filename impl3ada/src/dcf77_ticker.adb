-- mainloop_timing.c
package body DCF77_Ticker is

	procedure Init(Ctx: in out Ticker; LL: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL       := LL;
		Ctx.Time_Old := LL.Get_Time_Micros;
	end Init;

	procedure Tick(Ctx: in out Ticker) is
		Time_New: constant Time := Ctx.LL.Get_Time_Micros;
		Delta_T:  constant Time := Time_New - Ctx.Time_Old;
	begin
		if Delta_T < (Delay_Us_Target - Delay_Us_Variance) then
			Ctx.Delay_Us := Ctx.Delay_Us +
				(Delay_Us_Target - Delta_T) / Delay_Slowdown;
		elsif Delta_T > (Delay_Us_Target + Delay_Us_Variance) then
			Ctx.Delay_Us := Ctx.Delay_Us - Time'Min(
				Ctx.Delay_Us + 1,
				(Delta_T - Delay_Us_Target) / Delay_Slowdown);
		end if;
		Ctx.Time_Old := Time_New;
		Ctx.LL.Delay_Micros(Ctx.Delay_Us);
	end Tick;

end DCF77_Ticker;
