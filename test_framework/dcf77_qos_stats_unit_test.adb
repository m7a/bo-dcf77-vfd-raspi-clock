with DCF77_QOS_Stats;
use  DCF77_QOS_Stats;
with DCF77_Test_Support;
use  DCF77_Test_Support; -- Test_Pass/Test_Fail

package body DCF77_QOS_Stats_Unit_Test is

	-- Test Counter Resets After Day!
	procedure Run is
		Stats: QOS_Stats;

		procedure Expect_Report(Expect_R: in String) is
			Got_R: constant String := Stats.Format_Report;
		begin
			if Expect_R = Got_R then
				Test_Pass("Fault Counter Resets After Day - " &
						Expect_R);
			else
				Test_Fail("Fault Counter Resets After Day - " &
						Expect_R & " - got " & Got_R &
						" instead");
			end if;
		end Expect_Report;

		procedure Skip_Day is
		begin
			for I in 1 .. (3600 * 24) loop
				Stats.Inc(Good);
			end loop;
		end Skip_Day;
	begin
		Stats.Init;

		-- bad is added to second column
		Stats.Inc(Bad);
		Expect_Report("0000 0001 0000");

		Stats.Inc(Bad);
		Expect_Report("0000 0002 0000");

		-- at EOD transfer to last one independently of number of faults
		Skip_Day;
		Expect_Report("0000 0000 0001");

		-- intermediate is added to first columen
		Stats.Inc(Intermediate);
		Expect_Report("0001 0000 0001");

		-- both types of error result in same outcome
		Skip_Day;
		Expect_Report("0000 0000 0002");

		-- no event means no inc
		Skip_Day;
		Expect_Report("0000 0000 0002");
	end Run;

end DCF77_QOS_Stats_Unit_Test;
