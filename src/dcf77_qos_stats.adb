with DCF77_Functions;
use  DCF77_Functions;

package body DCF77_QOS_Stats is

	procedure Init(Ctx: in out QOS_Stats) is
	begin
		Ctx.Timer                  := 0;
		Ctx.Ctr_Intermediate       := 0;
		Ctx.Ctr_Bad                := 0;
		Ctx.Ctr_Days_With_Non_Good := 0;
	end Init;

	procedure Inc(Ctx: in out QOS_Stats; Report: in Generic_QOS_Level) is
	begin
		Ctx.Timer := Ctx.Timer + 1;
		if Ctx.Timer >= Length_Of_Day then
			if Ctx.Ctr_Intermediate /= 0 or Ctx.Ctr_Bad /= 0 then
				Inc_Saturated(Ctx.Ctr_Days_With_Non_Good, 9999);
				Ctx.Ctr_Intermediate := 0;
				Ctx.Ctr_Bad          := 0;
			end if;
			Ctx.Timer := 0;
		end if;
		case Report is
		when Good         => null; -- nothing to record
		when Intermediate => Inc_Saturated(Ctx.Ctr_Intermediate, 9999);
		when Bad          => Inc_Saturated(Ctx.Ctr_Bad, 9999);
		end case;
	end Inc;

	function Format_Report(Ctx: in QOS_Stats) return String is
				(Num_To_Str_L4(Ctx.Ctr_Intermediate) & " " &
				 Num_To_Str_L4(Ctx.Ctr_Bad)          & " " &
				 Num_To_Str_L4(Ctx.Ctr_Days_With_Non_Good));

end DCF77_QOS_Stats;
