with DCF77_Low_Level;
with DCF77_Timelayer;

package DCF77_Alarm is

	type Alarm is tagged limited private;

	-- subset of TM
	type Time_T is record
		H: Natural; -- 0..23
		I: Natural; -- 0..59
		S: Natural; -- here often 0
	end record;

	procedure Init(Ctx: in out Alarm; LLI: in DCF77_Low_Level.LLP);
	procedure Process(Ctx: in out Alarm;
					DT_Now_Prime: in DCF77_Timelayer.TM);

	-- GUI API
	function Get_AL_Time(Ctx: in Alarm) return Time_T;
	function Is_Alarm_Enabled(Ctx: in Alarm) return Boolean;
	procedure Set_AL_Time(Ctx: in out Alarm; T: in Time_T);

private

	-- Timeout buzzing after 1.5h as to protect neighbors from alarm clocks
	-- going rogue. Also, if no waekup within that timeframe the appointment
	-- is likely to be missed anyways...
	Buzz_Timeout_Seconds: constant Natural := 5400;

	type Blink is mod 6;

	type State is (AL_Start, AL_Buzz, AL_Timeout, AL_Disabled, AL_Check);

	-- subset of TM
	type Date_T is record
		Y: Natural;
		M: Natural;
		D: Natural;
	end record;

	type Alarm is tagged limited record
		LL: DCF77_Low_Level.LLP;

		-- User-configured alarm time
		T_AL: Time_T;

		-- Record if GUI just performed some changes to T_AL while
		-- we were processing...
		AL_Has_Changed: Boolean;

		-- Primary state of the alarm automaton
		S:  State;

		-- Store Date and Time of when the alarm checking started
		DT_Now: DCF77_Timelayer.TM;

		-- While buzzing this specifies a timeout after which the
		-- buzzing should halt i.e. switch to AL_Timeout.
		DT_Stop_Buzz: DCF77_Timelayer.TM;

		-- Maintain state for blinkin in AL_Buzz and AL_Timeout states
		Blink_CTR: Blink;
	end record;

	procedure Start_Buzzing(Ctx: in out Alarm;
					DT_Now_Prime: in DCF77_Timelayer.TM);
	procedure Run_Blink(Ctx: in out Alarm);
	function Check_For_Time_Of_Alarm(Ctx: in out Alarm;
			DT_Now_Prime: in DCF77_Timelayer.TM) return Boolean;

	function ">="(A, B: in DCF77_Timelayer.TM) return Boolean;
	function "<"(A, B: in DCF77_Timelayer.TM) return Boolean;
	function Date(T: in DCF77_Timelayer.TM) return Date_T;
	function Time(T: in DCF77_Timelayer.TM) return Time_T;
	function ">="(A, B: in Time_T) return Boolean;

end DCF77_Alarm;
