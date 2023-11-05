package body DCF77_Timelayer.Testing is

	function Are_Ones_Compatible(AD, BD: in BCD_Digit) return Boolean is
				(DCF77_Timelayer.Are_Ones_Compatible(AD, BD));
	function Is_Leap_Year(Y: in Natural) return Boolean is
				(DCF77_Timelayer.Is_Leap_Year(Y));

	procedure Advance_TM_By_Sec(T: in out TM; Seconds: in Natural) is
	begin
		DCF77_Timelayer.Advance_TM_By_Sec(T, Seconds);
	end Advance_TM_By_Sec;

	function Test_Recover_Ones(Preceding_Minute_Ones: in Minute_Buf;
					Preceding_Minute_Idx: in Minute_Buf_Idx)
					return Integer is
		TTL: Timelayer;
	begin
		TTL.Init;
		TTL.Preceding_Minute_Ones := Preceding_Minute_Ones;
		TTL.Preceding_Minute_Idx  := Preceding_Minute_Idx;
		return TTL.Recover_Ones;
	end Test_Recover_Ones;

	function Decode(Tel: in Telegram) return TM is
					(DCF77_Timelayer.Decode(Tel));
	function TM_To_Telegram(T: in TM) return Telegram is
					(DCF77_Timelayer.TM_To_Telegram(T));

end DCF77_Timelayer.Testing;
