with DCF77_Functions;
use  DCF77_Functions; -- x Num_To_Str_L2, L4
with DCF77_Types;
with DCF77_Low_Level;

with Interfaces;
use  Interfaces;

use type DCF77_Types.Reading;
use type DCF77_Low_Level.Time;

package body DCF77_GUI is

	------------------------------------------------------------------------
	------------------------------------------------------------------------
	-----------------------------------------                             --
	-----------------------------------------  PROGRAM ENTRYPOINT “MAIN”  --
	-----------------------------------------                             --
	------------------------------------------------------------------------
	------------------------------------------------------------------------

	procedure Mainloop is
		G: aliased GUI;

		Date_S:       String := "20YYMMDD";
		Time_S:       String := "HH:ii:ss";
		Date_B:       SB.Bounded_String;
		Time_B:       SB.Bounded_String;
		Last_Reading: DCF77_Types.Reading := DCF77_Types.No_Update;

	begin
		G.S.Init;
		G.Init;

		loop
			G.S.Loop_Pre;
			G.Process; -- TODO SATISFY COMPILER WARNING FOR NOW

			--Date_S := Nat_To_S(Datetime.Y, 4) & "-" &
			--		Nat_To_S(Datetime.M, 2) & "-" &
			--		Nat_To_S(Datetime.D, 2);
			Date_S := Num_To_Str_L4(G.S.Datetime.Y) &
					Num_To_Str_L2(G.S.Datetime.M) &
					Num_To_Str_L2(G.S.Datetime.D);
			Time_S := Num_To_Str_L2(G.S.Datetime.H) & ":" &
					Num_To_Str_L2(G.S.Datetime.I) & ":" &
					Num_To_Str_L2(G.S.Datetime.S);
			Date_B := SB.To_Bounded_String(Date_S);
			Time_B := SB.To_Bounded_String(Time_S);

			if G.S.Bitlayer_Reading /= DCF77_Types.No_Update then
				Last_Reading := G.S.Bitlayer_Reading;
			end if;

			G.S.Disp.Update((
				1 => (X => 0, Y => 0, F => Small, Msg => Date_B,
					others => <>),
				2 => (X => 68, Y => 0, F => Small,
					Msg => SB.To_Bounded_String(
					DCF77_Low_Level.Time'Image(
					G.S.Ticker.Get_Delay / 1000)),
					others => <>),
				3 => (X => 100, Y => 0, F => Small,
					Msg => Describe_QOS(G.S.Timelayer.Get_Quality_Of_Service),
					others => <>),
				4 => (X => 80, Y => 32, F => Small,
					Msg => SB.To_Bounded_String(
					Natural'Image(G.S.Bitlayer.Get_Unidentified)),
					others => <>),
				5 => (X => 0, Y => 48, F => Small,
					Msg => SB.To_Bounded_String(
					DCF77_Types.Reading'Image(Last_Reading) &
					" FL" & Natural'Image(G.S.LL.Get_Fault)),
					others => <>),
				6 => (X => 0, Y => 16, F => Large,
					Msg => Time_B, others => <>)
			));

			G.S.Loop_Post;
		end loop;
	end Mainloop;

	------------------------------------------------------------------------
	------------------------------------------------------------------------
	----------------------------------------                              --
	----------------------------------------  PROGRAM STATE MAINTAINANCE  --
	----------------------------------------                              --
	------------------------------------------------------------------------
	------------------------------------------------------------------------

	procedure Init(S: in out Program_State) is
	begin
		S.LL := LLI'Access;

		S.LL.Init;
		S.Disp.Init(S.LL);
		S.Ticker.Init(S.LL);

		S.Bitlayer.Init(S.LL);
		S.Secondlayer.Init;
		S.Timelayer.Init;

		S.Alarm.Init(S.LL);

		-- TODO x DEBUG ONLY
		S.LL.Log("BEFORE CTR=39");
		S.Disp.Update((1 => (X => 16, Y => 16, F => Small,
				Msg => SB.To_Bounded_String(
				"INIT CTR=39"), others => <>)));
	end Init;

	procedure Loop_Pre(S: in out Program_State) is
		Bitlayer_Has_New: Boolean;
	begin
		S.LL.Debug_Dump_Interrupt_Info;

		S.Bitlayer_Reading := S.Bitlayer.Update;
		S.Secondlayer.Process(S.Bitlayer_Reading,
			S.Secondlayer_Telegram_1, S.Secondlayer_Telegram_2);

		Bitlayer_Has_New := S.Bitlayer_Reading /= DCF77_Types.No_Update;

		-- TODO HEAVY DRIFT WHEN RUN W/O SYNC! NEED TO ADJUST THIS SOMEHOW...
		-- In principle we have to adjust the bitlayer to query
		-- one earlier when the preceding output was No_Signal.
		-- Since that was after 11 iterations, next check after
		-- 9 iterations to sum up to 20 i.e. 2000ms or something.

		if Bitlayer_Has_New then
			S.Timelayer.Process(True, -- Bitlayer_Has_New,
						S.Secondlayer_Telegram_1,
						S.Secondlayer_Telegram_2);
		end if;

		S.Datetime := S.Timelayer.Get_Current;

		-- process all of the time for blinking and user input
		-- handling!
		S.Alarm.Process(S.Datetime);
	end Loop_Pre;

	procedure Loop_Post(S: in out Program_State) is
	begin
		S.Ticker.Tick; -- last operation before next iteration
	end Loop_Post;

	------------------------------------------------------------------------
	------------------------------------------------------------------------
	----------------------------------------                              --
	----------------------------------------  GUI IMPLEMENTATION DETAILS  --
	----------------------------------------                              --
	------------------------------------------------------------------------
	------------------------------------------------------------------------

	procedure Init(G: in out GUI) is
	begin
		G.A                       := Select_Display;
		G.Numeric_Editing_Enabled := False;
		G.Blink_Value             := False;
		G.Screen := (others => (X => 0, Y => 0, 
				Msg => SB.To_Bounded_String(""), others => <>));
	end Init;

	procedure Process(G: in out GUI) is
	begin
		G.Process_Inputs;
		G.Screen_Idx := 0;
		G.Update_Screen;
		G.S.Disp.Update(G.Screen(G.Screen'First .. G.Screen_Idx));
	end Process;

	-- TODO IMPLEMENT PROCESSING HERE! MAY HAVE DEDICATED VARIANT FOR
	-- 	NUMERIC EDITING MODE?
	procedure Process_Inputs(G: in out GUI) is
	begin
		case G.A is
		when Select_Display =>
			null;
		when Select_Alarm =>
			null;
		when Select_AL_H =>
			null;
		when Select_AL_I =>
			null;
		when Select_Datetime =>
			null;
		when Select_DT_H =>
			null;
		when Select_DT_I =>
			null;
		when Select_DT_S =>
			null;
		when Select_DT_YH =>
			null;
		when Select_DT_Y =>
			null;
		when Select_DT_M =>
			null;
		when Select_DT_D =>
			null;
		when Select_Options =>
			null;
		when Select_Info =>
			null;
		end case;
	end Process_Inputs;

	procedure Update_Screen(G: in out GUI) is
	begin
		case G.A is
		when Select_Display =>
			G.Add_Time(0, 16, Large, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(48);
			if G.S.Alarm.Is_Alarm_Enabled then
				G.Add_AL(48, Underline_None);
			end if;
		-- <alarm>
		when Select_Alarm =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_BTLR);
			G.Add_Menu(Menu_Next);
		when Select_AL_H =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, 2);
			G.Add_Menu(Menu_Edit);
		when Select_AL_I =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, 4);
			G.Add_Menu(Menu_Edit);
		-- </alarm> | <datetime>
		when Select_Datetime =>
			G.Add_Time(0, 16, Small, Underline_BLR);
			G.Add_Date(Underline_TLR);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Next);
		when Select_DT_H =>
			G.Add_Time(0, 16, Small, 1);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_I =>
			G.Add_Time(0, 16, Small, 3);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_S =>
			G.Add_Time(0, 16, Small, 5);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_YH =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(1);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_Y =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(2);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_M =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(4);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_D =>
			G.Add_Time(0, 16, Small, Underline_None);
			G.Add_Date(6);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		-- </datetime> | <options>
		when Select_Options =>
			G.Add_Time(0, 16, Small, Underline_None);
			-- TODO NOT SUPPORTED YET
			G.Add_Menu(Menu_Next);
		-- </options> | <info>
		when Select_Info =>
			G.Add_Time(0, 16, Small, Underline_None);
			-- TODO NOT SUPPORTED YET
			G.Add_Menu(Menu_Next);
		end case;
	end Update_Screen;

	-- H (1), M (3), S (5)
	procedure Add_Time(G: in out GUI; XI: in Pos_X; YI: in Pos_Y;
				FI: in Font; Underline: in Underline_Info) is
		CW: constant Pos_X := (if FI = Small then 8 else 16);
		SP: constant SB.Bounded_String := SB.To_Bounded_String(":");
		IU: Natural := G.Screen_Idx;
	begin
		-- H (1)
		if not (Underline = 1 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI, Y => YI, F => FI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.H)),
					ULB => Underline = 1 or
						Underline = Underline_BLR,
					ULL => Underline = Underline_BLR,
					others => <>);
		end if;
		-- Sep (2)
		IU           := IU + 1;
		G.Screen(IU) := (X => XI + CW * 2, Y => YI, F => FI, Msg => SP,
				ULB => Underline = Underline_BLR, others => <>);
		-- I (3)
		if not (Underline = 3 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI + CW * 3, Y => YI, F => FI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.I)),
					ULB => Underline = 3 or
						Underline = Underline_BLR,
					others => <>);
		end if;
		-- Sep (4)
		IU           := IU + 1;
		G.Screen(IU) := (X => XI + CW * 5, Y => YI, F => FI, Msg => SP,
				ULB => Underline = Underline_BLR, others => <>);
		-- S (5)
		if not (Underline = 5 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI + CW * 6, Y => YI, F => FI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.H)),
					ULB => Underline = 5 or
						Underline = Underline_BLR,
					ULR => Underline = Underline_BLR,
					others => <>);
		end if;
		G.Screen_Idx := IU;
	end Add_Time;

	-- YH: 1, Y:  2, M:  4, D:  6
	procedure Add_Date(G: in out GUI; Underline: in Underline_Info) is
		SP: constant SB.Bounded_String := SB.To_Bounded_String("-");
		IU:          Natural           := G.Screen_Idx;
	begin
		-- YH (1)
		if not (Underline = 1 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => 0, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.Y / 100)),
				ULB => Underline = 1,
				ULT | ULL => Underline = Underline_TLR,
				others => <>);
		end if;
		-- Y (2)
		if not (Underline = 2 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => 16, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.Y mod 100)),
				ULB => Underline = 2,
				ULT => Underline = Underline_TLR, others => <>);
		end if;
		-- Sep (3)
		IU           := IU + 1;
		G.Screen(IU) := (X => 32, Y => 0, Msg => SP,
				ULT => Underline = Underline_TLR, others => <>);
		-- M (4)
		if not (Underline = 4 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => 40, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.M)),
				ULB => Underline = 4,
				ULT => Underline = Underline_TLR, others => <>);
		end if;
		-- Sep (5)
		IU           := IU + 1;
		G.Screen(IU) := (X => 56, Y => 0, Msg => SP,
				ULT => Underline = Underline_TLR, others => <>);
		-- D (6)
		if not (Underline = 6 and G.Blink_Value) then
			IU           := IU + 1;
			G.Screen(IU) := (X => 64, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.D)),
				ULB => Underline = 6,
				ULT | ULR => Underline = Underline_TLR,
				others => <>);
		end if;
		G.Screen_Idx := IU;
	end Add_Date;

	procedure Add_QOS(G: in out GUI; YI: in Pos_Y) is
	begin
		G.Screen_Idx := G.Screen_Idx + 1;
		G.Screen(G.Screen_Idx) := (X => 0, Y => YI, Msg => Describe_QOS(
					G.S.Timelayer.Get_Quality_Of_Service),
				others => <>);
	end Add_QOS;

	procedure Add_AL(G: in out GUI; YI: in Pos_Y;
					Underline: in Underline_Info) is
		AL: constant DCF77_Alarm.Time_T := G.S.Alarm.Get_AL_Time;
		IU: Natural := G.Screen_Idx;
	begin
		-- “AL” Prefix (1)
		IU := IU + 1;
		G.Screen(IU) := (X => 48, Y => YI,
				Msg => SB.To_Bounded_String("AL"),
				ULB | ULL | ULT => Underline = Underline_BTLR,
				others => <>);
		-- H (2)
		if not (Underline = 2 and G.Blink_Value) then
			IU := IU + 1;
			G.Screen(IU) := (X => 64, Y => YI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(AL.H)),
					ULB => Underline = 2 or
						Underline = Underline_BTLR,
					ULT => Underline = Underline_BTLR,
					others => <>);
		end if;
		-- Sep (3)
		IU := IU + 1;
		G.Screen(IU) := (X => 80, Y => YI,
					Msg => SB.To_Bounded_String(":"),
					ULB | ULT => Underline = Underline_BTLR,
					others => <>);
		-- I (4)
		if not (Underline = 4 and G.Blink_Value) then
			IU := IU + 1;
			G.Screen(IU) := (X => 88, Y => YI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(AL.I)),
					ULB => Underline = 4 or
						Underline = Underline_BTLR,
					ULT | ULR => Underline = Underline_BTLR,
					others => <>);
		end if;
		G.Screen_Idx := IU;
	end Add_AL;

	function Describe_QOS(Q: in DCF77_Timelayer.QOS)
						return SB.Bounded_String is
		use DCF77_Timelayer;
		QOS_Descr: constant array (QOS) of SB.Bounded_String := (
			QOS1       => SB.To_Bounded_String("+1"),
        		QOS2       => SB.To_Bounded_String("+2"),
        		QOS3       => SB.To_Bounded_String("+3"),
        		QOS4       => SB.To_Bounded_String("o4"),
        		QOS5       => SB.To_Bounded_String("o5"),
        		QOS6       => SB.To_Bounded_String("o6"),
        		QOS7       => SB.To_Bounded_String("-7"),
        		QOS8       => SB.To_Bounded_String("-8"),
        		QOS9_ASYNC => SB.To_Bounded_String("-9")
		);
	begin
		return QOS_Descr(Q);
	end Describe_QOS;

	procedure Add_Menu(Q: in out GUI; Lbl: in Menu_Green) is
	begin
		-- TODO ASTAT ... Draw menu here
		null;
	end Add_Menu;

end DCF77_GUI;
