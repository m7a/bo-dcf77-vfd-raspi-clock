with DCF77_TM_Layer_Shared;

package DCF77_Minutelayer_Unit_Test is

	procedure Run;

private

	type Tel_Conv_TV is record
		Tel:  String(1 .. 60);
		Time: DCF77_TM_Layer_Shared.TM;
	end record;

	procedure Test_Are_Ones_Compatible;
	procedure Test_Is_Leap_Year;
	procedure Test_Advance_TM_By_Sec;
	procedure Test_Recover_Ones;
	procedure Test_Decode;
	procedure Test_Telegram_Identity;

end DCF77_Minutelayer_Unit_Test;
