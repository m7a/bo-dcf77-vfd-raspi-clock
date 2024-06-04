package DCF77_Minutelayer.Testing is

	function Are_Ones_Compatible(AD, BD: in BCD_Digit) return Boolean;
	function Test_Recover_Ones(Preceding_Minute_Ones: in Minute_Buf;
			Preceding_Minute_Idx: in Minute_Buf_Idx) return Integer;
	function Decode(Tel: in Telegram) return TM;
	function TM_To_Telegram(T: in TM) return Telegram;

end DCF77_Minutelayer.Testing;
