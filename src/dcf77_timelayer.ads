with DCF77_TM_Layer_Shared;
use  DCF77_TM_Layer_Shared;

package DCF77_Timelayer is

	type Timelayer is tagged limited private;

	procedure Init(Ctx: in out Timelayer);
	procedure Process(Ctx: in out Timelayer; Exch: in TM_Exchange);
	function Get_Current(Ctx: in Timelayer) return TM;

	-- GUI interaction.
	-- Set_TM_By_User_Input _must_ be sent to Timelayer, too!
	procedure Set_TM_By_User_Input(Ctx: in out Timelayer; T: in TM);
	function Is_DCF77_Enabled(Ctx: in Timelayer) return Boolean;
	procedure Set_DCF77_Enabled(Ctx: in out Timelayer; En: in Boolean);
	function Get_QOS_Sym(Ctx: in Timelayer) return Character;

private

	type Timelayer is tagged limited record
		-- Bypass processing except for date & time computation based
		-- on preceding value. Can be used to operate the clock manually
		-- and for time drift debugging purposes.
		DCF77_Enabled: Boolean;

		-- Accept as initialization until at least one confident
		-- output is reported
		Is_Init: Boolean;

		-- Whose seconds do we prefer:
		-- The minutelayer's or our own computed ones?
		Prefer_Seconds_From_Minutelayer: Boolean;

		QOS_Sym: Character; -- I/+/o/- QOS symbology

		Last:   TM;      -- What we received from Timelayer
		Ctr:    Natural; -- How many TM from Timelayer were consistent
		Before: TM;      -- What we output
	end record;

	function Are_Minutes_Equal(A, B: in TM) return Boolean;

	procedure Process_New_Minute(Ctx: in out Timelayer;
					Exch: in TM_Exchange; Old: in TM);
	procedure Set_Last_And_Count(Ctx: in out Timelayer; Proposed: in TM);

end DCF77_Timelayer;
