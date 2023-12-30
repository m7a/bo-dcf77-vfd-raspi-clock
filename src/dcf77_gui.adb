with DCF77_Functions;
use  DCF77_Functions; -- x Num_To_Str_L2, L4
with DCF77_Types;

with Interfaces;
use  Interfaces;

use type DCF77_Types.Reading;

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
	begin
		G.S.Init;
		G.Init;
		loop
			G.S.Loop_Pre;
			G.Process;
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
		S.LL.Log("BEFORE CTR=41");
		S.Disp.Update((1 => (X => 16, Y => 16, F => Small,
				Msg => SB.To_Bounded_String(
				"INIT CTR=41"), others => <>)));
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
		G.Blink_Value             := 0;
		G.Screen := (others => (X => 0, Y => 0, 
				Msg => SB.To_Bounded_String(""), others => <>));
		G.RFU_Option              := True;
	end Init;

	procedure Process(G: in out GUI) is
		Green: constant Boolean := G.S.LL.Read_Green_Button_Is_Down;
		Left:  constant Boolean := G.S.LL.Read_Left_Button_Is_Down;
		Right: constant Boolean := G.S.LL.Read_Right_Button_Is_Down;
	begin
		if G.Numeric_Editing_Enabled then
			G.Process_Editing_Inputs(Green, Left, Right);
		else
			G.Process_Navigation_Inputs(Green, Left, Right);
		end if;

		G.Screen_Idx := 0;
		G.Update_Screen;
		G.S.Disp.Update(G.Screen(G.Screen'First .. G.Screen_Idx));
	end Process;

	procedure Process_Editing_Inputs(G: in out GUI;
					Finish, Left, Right: in Boolean) is

		function Edit(Field: in out Natural; Modulus: in Natural)
							return Boolean is
		begin
			if Left and Right then
				Field := 0;
				return True;
			elsif Left then
				Field := (if Field = 0 then Modulus - 1
								else Field - 1);
				return True;
			elsif Right then
				Field := (Field + 1) mod Modulus;
				return True;
			else
				-- else do nothing
				return False;
			end if;
		end Edit;

		function Edit_AL_Group return Boolean is
			CAL: DCF77_Alarm.Time_T := G.S.Alarm.Get_AL_Time;
			Has_Changes: Boolean;
		begin
			case G.A is
			when Select_AL_H => Has_Changes := Edit(CAL.H, 24);
			when Select_AL_I => Has_Changes := Edit(CAL.I, 60);
			when others      => return False;
			end case;
			if Has_Changes then
				G.S.Alarm.Set_AL_Time(CAL);
			end if;
			return True;
		end Edit_AL_Group;

		function Edit_Datetime_Group return Boolean is
			CDT:         DCF77_Timelayer.TM := G.S.Datetime;
			YH:          Natural            := CDT.Y  /  100;
			Y:           Natural            := CDT.Y mod 100;
			Has_Changes: Boolean;
		begin
			case G.A is
			when Select_DT_H =>
				Has_Changes := Edit(CDT.H, 24);
			when Select_DT_I =>
				Has_Changes := Edit(CDT.I, 60);
			when Select_DT_S =>
				-- Disallow input of leap second through GUI for
				-- now
				Has_Changes := Edit(CDT.S, 60);
			when Select_DT_YH =>
				Has_Changes := Edit(YH, 100);
				CDT.Y       := Y + YH * 100;
			when Select_DT_Y =>
				Has_Changes := Edit(Y, 100);
				CDT.Y       := Y + YH * 100;
			when Select_DT_M =>
				Has_Changes := Edit(CDT.M, 12);
			when Select_DT_D =>
				-- Currently requires that the day is valid
				-- within month. Should be OK due to Y-M-D order
				-- of editing!
				Has_Changes := Edit(CDT.D,
					DCF77_Timelayer.Month_Lengths(if
						CDT.M = 2 and DCF77_Timelayer.
							Is_Leap_Year(CDT.Y)
						then 0 else CDT.M));
			when others =>
				return False;
			end case;
			if Has_Changes then
				G.S.Timelayer.Set_TM_By_User_Input(CDT);
			end if;
			return True;
		end Edit_Datetime_Group;
	begin
		if Finish then
			G.Numeric_Editing_Enabled := False;
			G.Blink_Value             := 0;
		else
			G.Blink_Value := G.Blink_Value + 1; -- Blink!
			if not (Edit_AL_Group or else Edit_Datetime_Group) then
				-- invalid input, return to default screen
				G.Numeric_Editing_Enabled := False;
				G.A                       := Select_Display;
			end if;
		end if;
	end Process_Editing_Inputs;

	procedure Process_Navigation_Inputs(G: in out GUI;
					Next_Edit, Left, Right: in Boolean) is
	begin
		case G.A is
		-- generalized inner navigation --
		when Select_AL_H | Select_DT_H .. Select_DT_M =>
			if Next_Edit then
				G.Numeric_Editing_Enabled := True;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		-- display --
		when Select_Display =>
			if Next_Edit then
				G.A := Select_Alarm;
			end if;
		-- alarm --
		when Select_Alarm =>
			if Next_Edit then
				G.A := Select_Datetime;
			elsif Left then
				G.A := Select_Display;
			elsif Right then
				G.A := Select_AL_H;
			end if;
		when Select_AL_I =>
			if Next_Edit then
				G.Numeric_Editing_Enabled := True;
			elsif Left then
				G.A := Select_AL_H;
			elsif Right then
				G.A := Select_Display;
			end if;
		-- datetime --
		when Select_Datetime =>
			if Next_Edit then
				G.A := Select_Options;
			elsif Left then
				G.A := Select_Display;
			elsif Right then
				G.A := Select_DT_H;
			end if;
		when Select_DT_D =>
			if Next_Edit then
				G.Numeric_Editing_Enabled := True;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := Select_Display;
			end if;
		-- options --
		when Select_Options =>
			if Next_Edit then
				G.A := Select_Info;
			elsif Left then
				G.A := Select_Display;
			elsif Right then
				G.A := Select_OPT_DCF77_EN;
			end if;
		when Select_OPT_DCF77_EN =>
			if Next_Edit then
				G.S.Timelayer.Set_DCF77_Enabled(
					not G.S.Timelayer.Is_DCF77_Enabled);
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		when Select_OPT_RFU_EN =>
			if Next_Edit then
				G.RFU_Option := not G.RFU_Option;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := Select_Display;
			end if;
		-- info --
		when Select_Info =>
			if Next_Edit then
				G.A := Select_Display;
			elsif Left then
				G.A := Select_Display;
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		when Select_I_QOS .. Select_I_Ver_2 =>
			if Next_Edit then
				G.A := Select_Display;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		when Select_I_Ver_3 =>
			if Next_Edit then
				G.A := Select_Display;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := Select_Display;
			end if;
		end case;
	end Process_Navigation_Inputs;

	procedure Update_Screen(G: in out GUI) is
		use type DCF77_Low_Level.Time;
	begin
		case G.A is
		when Select_Display =>
			G.Add_Time(0, 16, Large, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(48);
			if G.S.Alarm.Is_Alarm_Enabled then
				G.Add_AL(48, Underline_None);
			end if;
		-- alarm --
		when Select_Alarm =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_BTLR);
			G.Add_Menu(Menu_Next);
		when Select_AL_H =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, 2);
			G.Add_Menu(Menu_Edit);
		when Select_AL_I =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, 4);
			G.Add_Menu(Menu_Edit);
		-- datetime --
		when Select_Datetime =>
			G.Add_Time(32, 16, Small, Underline_BLR);
			G.Add_Date(Underline_TLR);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Next);
		when Select_DT_H =>
			G.Add_Time(32, 16, Small, 1);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_I =>
			G.Add_Time(32, 16, Small, 3);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_S =>
			G.Add_Time(32, 16, Small, 5);
			G.Add_Date(Underline_None);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_YH =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(1);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_Y =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(2);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_M =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(4);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_D =>
			G.Add_Time(32, 16, Small, Underline_None);
			G.Add_Date(6);
			G.Add_QOS(32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		-- options --
		when Select_Options =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Option_DCF77(Underline_TLR);
			G.Add_Option_RFU(Underline_BLR);
			G.Add_Menu(Menu_Next);
		when Select_OPT_DCF77_EN =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Option_DCF77(1);
			G.Add_Option_RFU(Underline_None);
			G.Add_Menu(Menu_Toggle);
		when Select_OPT_RFU_EN =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Option_DCF77(Underline_None);
			G.Add_Option_RFU(1);
			G.Add_Menu(Menu_Toggle);
		-- info --
		when Select_Info =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Info("CTRInfo",
				"LL " & Num_To_Str_L4(G.S.LL.Get_Fault) &
					" BIT" & Num_To_Str_L4(
					G.S.Bitlayer.Get_Unidentified),
				"SEC" & Num_To_Str_L4(G.S.Secondlayer.Get_Fault)
					& " DLY" & Num_To_Str_L4(Natural(
					G.S.Ticker.Get_Delay / 1000)));
			G.Add_Menu(Menu_Home);
		when Select_I_QOS =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Info("QOSInfo",
				"+1 23 45 678  9", G.S.Timelayer.Get_QOS_Stats);
			G.Add_Menu(Menu_Home);
		when Select_I_Last_1 =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Last_1;
			G.Add_Menu(Menu_Home);
		when Select_I_Last_2 =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Last_2;
			G.Add_Menu(Menu_Home);
		when Select_I_Ver_1 =>
			G.Add_Time(0, 0, Small, Underline_None);
			-- indentation exceeded
			G.Add_Info("Ver.1/3",
			"Version 01.00.00", "Date" &
			Num_To_Str_L4(DCF77_Timelayer.Time_Of_Compilation.Y) &
			Num_To_Str_L2(DCF77_Timelayer.Time_Of_Compilation.M) &
			Num_To_Str_L2(DCF77_Timelayer.Time_Of_Compilation.D) &
			Num_To_Str_L2(DCF77_Timelayer.Time_Of_Compilation.H) &
			Num_To_Str_L2(DCF77_Timelayer.Time_Of_Compilation.I));
			G.Add_Menu(Menu_Home);
		when Select_I_Ver_2 =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Info("Ver.2/3",
					"(c) 2018-2024", "    Ma_Sys.ma");
			G.Add_Menu(Menu_Home);
		when Select_I_Ver_3 =>
			G.Add_Time(0, 0, Small, Underline_None);
			G.Add_Info("Ver.3/3",
					"Further info:", "info@masysma.net");
			G.Add_Menu(Menu_Home);
		end case;
	end Update_Screen;

	-- H (1), M (3), S (5)
	procedure Add_Time(G: in out GUI; XI: in Pos_X; YI: in Pos_Y;
				FI: in Font; Underline: in Underline_Info) is
		CW: constant Pos_X             := Get_Letter_Width(FI);
		SP: constant SB.Bounded_String := SB.To_Bounded_String(":");
		IU:          Natural           := G.Screen_Idx;
	begin
		-- H (1)
		if not (Underline = 1 and G.Blink_Value > Blink_Lim) then
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
		if not (Underline = 3 and G.Blink_Value > Blink_Lim) then
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
		if not (Underline = 5 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI + CW * 6, Y => YI, F => FI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.S)),
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
		CX:          Pos_X             := 24;
	begin
		-- YH (1)
		if not (Underline = 1 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => CX, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.Y / 100)),
				ULB => Underline = 1,
				ULT | ULL => Underline = Underline_TLR,
				others => <>);
		end if;
		CX := CX + 16;
		-- Y (2)
		if not (Underline = 2 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => CX, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.Y mod 100)),
				ULB => Underline = 2,
				ULT => Underline = Underline_TLR, others => <>);
		end if;
		CX := CX + 16;
		-- Sep (3)
		IU           := IU + 1;
		G.Screen(IU) := (X => CX, Y => 0, Msg => SP,
				ULT => Underline = Underline_TLR, others => <>);
		CX := CX + 8;
		-- M (4)
		if not (Underline = 4 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => CX, Y => 0,
				Msg => SB.To_Bounded_String(
					Num_To_Str_L2(G.S.Datetime.M)),
				ULB => Underline = 4,
				ULT => Underline = Underline_TLR, others => <>);
		end if;
		CX := CX + 16;
		-- Sep (5)
		IU           := IU + 1;
		G.Screen(IU) := (X => CX, Y => 0, Msg => SP,
				ULT => Underline = Underline_TLR, others => <>);
		CX := CX + 8;
		-- D (6)
		if not (Underline = 6 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => CX, Y => 0,
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
		G.Screen(IU) := (X => 72, Y => YI,
				Msg => SB.To_Bounded_String("AL"),
				ULB | ULL | ULT => Underline = Underline_BTLR,
				others => <>);
		-- H (2)
		if not (Underline = 2 and G.Blink_Value > Blink_Lim) then
			IU := IU + 1;
			G.Screen(IU) := (X => 88, Y => YI,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(AL.H)),
					ULB => Underline = 2 or
						Underline = Underline_BTLR,
					ULT => Underline = Underline_BTLR,
					others => <>);
		end if;
		-- Sep (3)
		IU := IU + 1;
		G.Screen(IU) := (X => 104, Y => YI,
					Msg => SB.To_Bounded_String(":"),
					ULB | ULT => Underline = Underline_BTLR,
					others => <>);
		-- I (4)
		if not (Underline = 4 and G.Blink_Value > Blink_Lim) then
			IU := IU + 1;
			G.Screen(IU) := (X => 112, Y => YI,
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

	procedure Add_Menu(G: in out GUI; Lbl: in Menu_Green) is
		S_Minus: constant SB.Bounded_String := SB.To_Bounded_String(
			if G.Numeric_Editing_Enabled then " - " else " < ");
		S_Plus:  constant SB.Bounded_String := SB.To_Bounded_String(
			if G.Numeric_Editing_Enabled then " + " else " > ");
		IU: Natural := G.Screen_Idx;
	begin
		IU := IU + 1;
		G.Screen(IU) := (X => 0, Y => 48, ULT | ULR => True,
								others => <>);
		case Lbl is
		when Menu_Home =>
			G.Screen(IU).Msg := SB.To_Bounded_String("Home  ");
		when Menu_Next =>
			G.Screen(IU).Msg := SB.To_Bounded_String("Next  ");
		when Menu_Edit =>
			G.Screen(IU).Msg := SB.To_Bounded_String("Edit  ");
		when Menu_Toggle =>
			G.Screen(IU).Msg := SB.To_Bounded_String("Toggle");
		end case;
		IU := IU + 1;
		G.Screen(IU) := (X => 48, Y => 48, Msg => S_Minus,
					ULT | ULR => True, others => <>);
		IU := IU + 1;
		G.Screen(IU) := (X => 72, Y => 48, Msg => S_Plus,
					ULT | ULR => True, others => <>);
		IU := IU + 1;
		G.Screen(IU) := (X => 96, Y => 48, Msg => SB.To_Bounded_String(
					"ALM "), ULT => True, others => <>);
		G.Screen_Idx := IU;
	end Add_Menu;

	procedure Add_Option_DCF77(G: in out GUI;
						Underline: in Underline_Info) is
	begin
		G.Add_Option(Underline, 16, "DCF77 Proc.",
						G.S.Timelayer.Is_DCF77_Enabled);
	end Add_Option_DCF77;

	procedure Add_Option(G: in out GUI; Underline: in Underline_Info;
			YI: in Pos_Y; Label: in String; Value: in Boolean) is
		Yes: constant SB.Bounded_String := SB.To_Bounded_String("Yes");
		No:  constant SB.Bounded_String := SB.To_Bounded_String("No ");
		IU: Natural := G.Screen_Idx;
	begin
		IU := IU + 1;
		G.Screen(IU) := (X => 0, Y => YI,
				Msg => SB.To_Bounded_String(Label),
				others => <>);
		IU := IU + 1;
		G.Screen(IU) := (X => 88, Y => YI,
				Msg => (if Value then Yes else No),
				ULB => Underline = 1 or
					Underline = Underline_BLR,
				ULL | ULR => Underline = Underline_TLR or
					Underline = Underline_BLR,
				ULT => Underline = Underline_TLR,
				others => <>);
		G.Screen_Idx := IU;
	end Add_Option;

	procedure Add_Option_RFU(G: in out GUI; Underline: in Underline_Info) is
	begin
		G.Add_Option(Underline, 32, "RFU Option ", G.RFU_Option);
	end Add_Option_RFU;

	procedure Add_Info(G: in out GUI; Title, L1, L2: in String) is
		IU: Natural := G.Screen_Idx;
	begin
		IU := IU + 1;
		G.Screen(IU) := (X => 72, Y => 0, Msg =>
				SB.To_Bounded_String(Title), others => <>);
		IU := IU + 1;
		G.Screen(IU) := (X => 0, Y => 16, Msg =>
				SB.To_Bounded_String(L1), others => <>);
		IU := IU + 1;
		G.Screen(IU) := (X => 0, Y => 32, Msg =>
				SB.To_Bounded_String(L2), others => <>);
		G.Screen_Idx := IU;
	end Add_Info;

	procedure Add_Last_1(G: in out GUI) is
		L1: String(1 .. 16) := (others => ' ');
		L2: String(1 .. 16) := (others => ' ');
		Cursor_Passed_Out: Boolean;
	begin
		G.S.Secondlayer.Visualize_Bytes(L1, 0,  15, Cursor_Passed_Out);
		G.S.Secondlayer.Visualize_Bytes(L2, 16, 28, Cursor_Passed_Out);
		-- Hack to display dots if telegram continues on next page...
		if L2(13) /= ' ' and L2(13) /= '_' then
			L2(14 .. 16) := "...";
		end if;
		if Cursor_Passed_Out then
			G.A := Select_I_Last_2;
		end if;
		G.Add_Info("Last1/2", L1, L2);
	end Add_Last_1;

	procedure Add_Last_2(G: in out GUI) is
		L3: String(1 .. 16) := (others => ' ');
		L4: String(1 .. 15) := (others => ' ');
		Cursor_Passed_Out: Boolean;
	begin
		G.S.Secondlayer.Visualize_Bytes(L3, 29, 44, Cursor_Passed_Out);
		G.S.Secondlayer.Visualize_Bytes(L4, 45, 59, Cursor_Passed_Out);
		if Cursor_Passed_Out then
			G.A := Select_I_Last_1;
		end if;
		G.Add_Info("Last2/2", L3, L4);
	end Add_Last_2;

end DCF77_GUI;
