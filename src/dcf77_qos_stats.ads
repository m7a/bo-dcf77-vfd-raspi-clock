package DCF77_QOS_Stats is

	type Generic_QOS_Level is (Good, Intermediate, Bad);
	type QOS_Stats is tagged limited private;

	procedure Init(Ctx: in out QOS_Stats);
	procedure Inc(Ctx: in out QOS_Stats; Report: in Generic_QOS_Level);
	function Format_Report(Ctx: in QOS_Stats) return String;

private

	Length_Of_Day: constant Integer := 3600 * 24;
	Timer_Disabled: constant Integer := -1;

	type QOS_Stats is tagged limited record
		Timer:                  Integer;
		Ctr_Intermediate:       Natural;
		Ctr_Bad:                Natural;
		Ctr_Days_With_Non_Good: Natural;
	end record;

end DCF77_QOS_Stats;
