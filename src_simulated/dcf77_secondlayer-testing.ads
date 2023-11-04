package DCF77_Secondlayer.Testing is

	function Check_BCD_Correct_Telegram(
			Initial_Population: in DCF77_Types.Bits;
			Start_Offset_In_Line: in Natural) return Boolean;

	procedure Debug_Dump_State(Ctx: in Secondlayer);

end DCF77_Secondlayer.Testing;
