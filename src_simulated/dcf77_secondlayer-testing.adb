with Ada.Text_IO;
with DCF77_Offsets;
use  DCF77_Offsets;

package body DCF77_Secondlayer.Testing is

	function Check_BCD_Correct_Telegram(
			Initial_Population: in DCF77_Types.Bits;
			Start_Offset_In_Line: in Natural) return Boolean is
		Ctx: Secondlayer;
		Line: Line_Num := 0;
		Pos_In_Line: Natural := 0;
	begin
		Ctx.Init;
		for I of Initial_Population loop
			Ctx.Lines(Line).Value(Pos_in_Line) := I;
			Ctx.Lines(Line).Valid              := Valid_60;
			Pos_In_Line := Pos_In_Line + 1;
			if Pos_In_Line = Sec_Per_Min then
				Pos_In_Line := 0;
				Line        := Line + 1;
			end if;
		end loop;
		return Check_BCD_Correct_Telegram(Ctx, 0, Start_Offset_In_Line);
	end Check_BCD_Correct_Telegram;

	procedure Debug_Dump_State(Ctx: in Secondlayer) is
		procedure Print(S: in String) renames Ada.Text_IO.Put_Line;

		Bitmap: String(1 .. 62);
		K: Integer;
	begin
		Print("Inmode = " & Input_Mode'Image(Ctx.Inmode));
		Print("HasLIL = " & Boolean'Image(Ctx.Has_Leap_In_Line));
		Print("   LiL = " & Line_Num'Image(Ctx.Leap_In_Line));
		Print("LExpec = " & Natural'Image(Ctx.Leap_Second_Expected));
		Print("FaultR = " & Natural'Image(Ctx.Fault_Reset));
		Bitmap := (others => ' ');
		Bitmap(Ctx.Line_Cursor + 2) := 'v';
		Print(Bitmap);
		Bitmap(Ctx.Line_Cursor + 2) := ' ';
		for I in Ctx.Lines'Range loop
			Bitmap(Bitmap'First) :=
				(if I = Ctx.Line_Current then '>' else ' ');
			K := Bitmap'First + 1;
			for J of Ctx.Lines(I).Value loop
				case J is
				when Bit_0     => Bitmap(K) := '0';
				when Bit_1     => Bitmap(K) := '1';
				when No_Update => Bitmap(K) := '2';
				when No_Signal => Bitmap(K) := '3';
				end case;
				K := K + 1;
			end loop;
			Print(Bitmap);
		end loop;
	end Debug_Dump_State;

end DCF77_Secondlayer.Testing;
