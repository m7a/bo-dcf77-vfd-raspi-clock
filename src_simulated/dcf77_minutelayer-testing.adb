package body DCF77_Minutelayer.Testing is

	function Are_Ones_Compatible(AD, BD: in BCD_Digit) return Boolean is
				(DCF77_Minutelayer.Are_Ones_Compatible(AD, BD));

	function Test_Recover_Ones(Preceding_Minute_Ones: in Minute_Buf;
					Preceding_Minute_Idx: in Minute_Buf_Idx)
					return Integer is
		TTL: Minutelayer;
	begin
		TTL.Init;
		TTL.Preceding_Minute_Ones := Preceding_Minute_Ones;
		TTL.Preceding_Minute_Idx  := Preceding_Minute_Idx;
		return TTL.Recover_Ones;
	end Test_Recover_Ones;

	function Decode(Tel: in Telegram) return TM is
		TTL: Minutelayer;
	begin
		TTL.Init;
		return TTL.Decode(Tel);
	end Decode;

	function TM_To_Telegram(T: in TM) return Telegram is
					(DCF77_Minutelayer.TM_To_Telegram(T));

end DCF77_Minutelayer.Testing;
