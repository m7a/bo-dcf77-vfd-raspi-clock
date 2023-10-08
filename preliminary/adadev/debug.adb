

		--procedure Print_Debug is
		--	Debug_Str: String(1 .. 60);
		--begin
		--	for I in Debug_Str'Range loop
		--		case Get_Bit(I - 1) is
		--		when Bit_0 => Debug_Str(I) := '0';
		--		when Bit_1 => Debug_Str(I) := '1';
		--		when No_Update => Debug_Str(I) := '2';
		--		when No_Signal => Debug_Str(I) := '3';
		--		end case;
		--	end loop;
		--	Ada.Text_IO.Put_Line("  CHECKBCD: BoM=" & Inner_Checkresult'Image(Check_Begin_Of_Minute) & ", DST=" & Inner_Checkresult'Image(Check_DST) & ", BoT=" & Inner_Checkresult'Image(Check_Begin_Of_Time) & ", Min=" & Inner_Checkresult'Image(Check_Minute) & ", Hour=" & Inner_Checkresult'Image(Check_Hour) & ", Date=" & Inner_Checkresult'Image(Check_Date) & ", EoM=" & Inner_Checkresult'Image(Check_End_Of_Minute) & " --- " & Debug_Str);
		--end Print_Debug;
