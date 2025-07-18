with DCF77_Functions;
use  DCF77_Functions; -- x Num_To_Str_L2, L4
with DCF77_Types;
with DCF77_TM_Layer_Shared;
use  DCF77_TM_Layer_Shared;

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

		S.Bitlayer.Init(S.LL);
		S.Secondlayer.Init;
		S.Minutelayer.Init;
		S.Timelayer.Init;

		S.ALS.Init;
		S.Alarm.Init(S.LL);

		S.LL.Log("Ma_Sys.ma DCF77 VFD / INIT CTR=" &
					Num_To_Str_L3(Ctr_Of_Compilation));
		S.Disp.Update((1 => (X => 16, Y => 16, F => Small,
				Msg => SB.To_Bounded_String("INIT CTR=" &
					Num_To_Str_L3(Ctr_Of_Compilation)),
				others => <>)));
	end Init;

	procedure Loop_Pre(S: in out Program_State) is
		Bitlayer_Has_New: Boolean;
		Exch: DCF77_TM_Layer_Shared.TM_Exchange;
	begin
		S.LL.Debug_Dump_Interrupt_Info;

		S.Bitlayer_Reading := S.Bitlayer.Update_Tick;
		S.Secondlayer.Process(S.Bitlayer_Reading,
			S.Secondlayer_Telegram_1, S.Secondlayer_Telegram_2);

		Bitlayer_Has_New := S.Bitlayer_Reading /= DCF77_Types.No_Update;

		if Bitlayer_Has_New then
			S.Minutelayer.Process(True, -- Bitlayer_Has_New,
						S.Secondlayer_Telegram_1,
						S.Secondlayer_Telegram_2, Exch);
			S.Timelayer.Process(Exch);
		end if;

		S.Light_Sensor_Reading := S.LL.Read_Light_Sensor;
		S.ALS.Update(S.Light_Sensor_Reading, S.Brightness_Setting);

		S.Datetime := S.Timelayer.Get_Current;

		-- process all of the time for blinking and user input handling!
		S.Alarm.Process(S.Datetime);
	end Loop_Pre;

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
		G.Last_Green              := False;
		G.Last_Left               := False;
		G.Last_Right              := False;
		G.RFU_Option              := True;
	end Init;

	procedure Process(G: in out GUI) is
		Green: constant Boolean := G.S.LL.Read_Green_Button_Is_Down;
		Left:  constant Boolean := G.S.LL.Read_Left_Button_Is_Down;
		Right: constant Boolean := G.S.LL.Read_Right_Button_Is_Down;
	begin
		if G.Numeric_Editing_Enabled then
			G.Process_Editing_Inputs(
					Green and not G.Last_Green,
					Left, Right);
		else
			G.Process_Navigation_Inputs(
					Green and not G.Last_Green,
					Left  and not G.Last_Left,
					Right and not G.Last_Right);
		end if;

		G.Screen_Idx := 0;
		G.Update_Screen;
		G.S.Disp.Update(G.Screen(G.Screen'First .. G.Screen_Idx),
				G.S.Brightness_Setting);

		G.Last_Green := Green;
		G.Last_Left  := Left;
		G.Last_Right := Right;
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
			CDT:         TM      := G.S.Datetime;
			YH:          Natural := CDT.Y  /  100;
			Y:           Natural := CDT.Y mod 100;
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
				Has_Changes := Edit(CDT.D, Month_Lengths(if
					CDT.M = 2 and Is_Leap_Year(CDT.Y)
					then 0 else CDT.M));
			when others =>
				return False;
			end case;
			if Has_Changes then
				G.S.Timelayer.Set_TM_By_User_Input(CDT);
				G.S.Minutelayer.Set_TM_By_User_Input(CDT);
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
		procedure Left_Is_Back_Right_Is_Forward is
		begin
			if Next_Edit then
				G.A := Select_Display;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		end Left_Is_Back_Right_Is_Forward;

		procedure Left_Is_Back_Right_Is_Home is
		begin
			if Next_Edit then
				G.A := Select_Display;
			elsif Left then
				G.A := State'Pred(G.A);
			elsif Right then
				G.A := Select_Display;
			end if;
		end Left_Is_Back_Right_Is_Home;
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
				G.A := Select_Info;
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
		-- info --
		when Select_Info =>
			if Next_Edit then
				G.A := Select_Version;
			elsif Left then
				G.A := Select_Display;
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		when Select_I_QOS .. Select_I_Last_2 =>
			Left_Is_Back_Right_Is_Forward;
		when Select_I_Alarm =>
			Left_Is_Back_Right_Is_Home;
		-- version --
		when Select_Version =>
			if Next_Edit then
				G.A := Select_Display;
			elsif Left then
				G.A := Select_Display;
			elsif Right then
				G.A := State'Succ(G.A);
			end if;
		when Select_Ver_2 =>
			Left_Is_Back_Right_Is_Forward;
		when Select_Ver_3 =>
			Left_Is_Back_Right_Is_Home;
		end case;
	end Process_Navigation_Inputs;

	procedure Update_Screen(G: in out GUI) is
	begin
		case G.A is
		when Select_Display =>
			G.Add_Main_Display;
			G.Add_QOS(112, 0);
			if G.S.Alarm.Is_Alarm_Enabled then
				G.Add_AL(48, Underline_None);
			end if;
		-- alarm --
		when Select_Alarm =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_BTLR);
			G.Add_Menu(Menu_Next);
		when Select_AL_H =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(0, 32);
			G.Add_AL(32, 2);
			G.Add_Menu(Menu_Edit);
		when Select_AL_I =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(Underline_None);
			G.Add_QOS(0, 32);
			G.Add_AL(32, 4);
			G.Add_Menu(Menu_Edit);
		-- datetime --
		when Select_Datetime =>
			G.Add_Time_Small(32, 16, Underline_BLR);
			G.Add_Date(Underline_TLR);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Next);
		when Select_DT_H =>
			G.Add_Time_Small(32, 16, 1);
			G.Add_Date(Underline_None);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_I =>
			G.Add_Time_Small(32, 16, 3);
			G.Add_Date(Underline_None);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_S =>
			G.Add_Time_Small(32, 16, 5);
			G.Add_Date(Underline_None);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_YH =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(1);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_Y =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(2);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_M =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(4);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		when Select_DT_D =>
			G.Add_Time_Small(32, 16, Underline_None);
			G.Add_Date(6);
			G.Add_QOS(0, 32);
			G.Add_AL(32, Underline_None);
			G.Add_Menu(Menu_Edit);
		-- info --
		when Select_Info =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info_Ctr;
			G.Add_Menu(Menu_Next);
		when Select_I_QOS =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info("QOSInfo",
					"M " & G.S.Minutelayer.Get_QOS_Stats,
					"T " & G.S.Timelayer.Get_QOS_Stats);
			G.Add_Menu(Menu_Home);
		when Select_I_Bits =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info_Bits;
			G.Add_Menu(Menu_Home);
		when Select_I_Bits_Oszi =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info_Oszi;
			G.Add_Menu(Menu_Home);
		when Select_I_Last_1 =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Last_1;
			G.Add_Menu(Menu_Home);
		when Select_I_Last_2 =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Last_2;
			G.Add_Menu(Menu_Home);
		when Select_I_Alarm =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info_Alarm;
			G.Add_Menu(Menu_Home);
		-- version --
		when Select_Version =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info("Ver.1/3", Version_String, "Date" &
					Num_To_Str_L4(Time_Of_Compilation.Y) &
					Num_To_Str_L2(Time_Of_Compilation.M) &
					Num_To_Str_L2(Time_Of_Compilation.D) &
					Num_To_Str_L2(Time_Of_Compilation.H) &
					Num_To_Str_L2(Time_Of_Compilation.I));
			G.Add_Menu(Menu_Home);
		when Select_Ver_2 =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info("Ver.2/3", "(c) 2018-2025", "    Ma_Sys.ma");
			G.Add_Menu(Menu_Home);
		when Select_Ver_3 =>
			G.Add_Time_Small(0, 0, Underline_None);
			G.Add_Info("Ver.3/3",
					"Further info:", "info@masysma.net");
			G.Add_Menu(Menu_Home);
		end case;
	end Update_Screen;

	-- No further options, Main is always displayed the same way...
	procedure Add_Main_Display(G: in out GUI) is
		Str: constant String(1 .. 5) := Num_To_Str_L2(G.S.Datetime.H) &
					":" & Num_To_Str_L2(G.S.Datetime.I);
		MD:  constant String(1 .. 5) := Num_To_Str_L2(G.S.Datetime.M) &
					"/" & Num_To_Str_L2(G.S.Datetime.D);
		IU: Natural := G.Screen_Idx;
	begin
		-- Custom time format
		IU := IU + 1;
		G.Screen(IU) := (X => 0, Y => 0, F => Large, Msg =>
				SB.To_Bounded_String(Str), others => <>);

		IU := IU + 1;
		G.Screen(IU) := (X => 80, Y => 0, F => Small,
				Msg => SB.To_Bounded_String(Num_To_Str_L2(
				G.S.Datetime.S)), others => <>);

		-- Custom date format
		IU := IU + 1;
		G.Screen(IU) := (X => 88, Y => 16, F => Small,
				Msg => SB.To_Bounded_String(Num_To_Str_L4(
				G.S.Datetime.Y)), others => <>);
		IU := IU + 1;
		G.Screen(IU) := (X => 84, Y => 32, F => Small,
				Msg => SB.To_Bounded_String(MD), others => <>);

		G.Screen_Idx := IU;
	end Add_Main_Display;

	procedure Add_Time_Small(G: in out GUI; XI: in Pos_X; YI: in Pos_Y;
						Underline: in Underline_Info) is
		SP: constant SB.Bounded_String := SB.To_Bounded_String(":");
		IU:          Natural           := G.Screen_Idx;
	begin
		-- H (1)
		if not (Underline = 1 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI, Y => YI, F => Small,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.H)),
					ULB => Underline = 1 or
						Underline = Underline_BLR,
					ULL => Underline = Underline_BLR,
					others => <>);
		end if;
		-- Sep (2)
		IU           := IU + 1;
		G.Screen(IU) := (X => XI + Letter_Width * 2, Y => YI,
				F => Small, Msg => SP,
				ULB => Underline = Underline_BLR, others => <>);
		-- I (3)
		if not (Underline = 3 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI + Letter_Width * 3, Y => YI,
					F => Small,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.I)),
					ULB => Underline = 3 or
						Underline = Underline_BLR,
					others => <>);
		end if;
		-- Sep (4)
		IU           := IU + 1;
		G.Screen(IU) := (X => XI + Letter_WIdth * 5, Y => YI,
				F => Small, Msg => SP,
				ULB => Underline = Underline_BLR, others => <>);
		-- S (5)
		if not (Underline = 5 and G.Blink_Value > Blink_Lim) then
			IU           := IU + 1;
			G.Screen(IU) := (X => XI + Letter_Width * 6, Y => YI,
					F => Small,
					Msg => SB.To_Bounded_String(
						Num_To_Str_L2(G.S.Datetime.S)),
					ULB => Underline = 5 or
						Underline = Underline_BLR,
					ULR => Underline = Underline_BLR,
					others => <>);
		end if;
		G.Screen_Idx := IU;
	end Add_Time_Small;

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

	procedure Add_QOS(G: in out GUI; XI: in Pos_X; YI: in Pos_Y) is
		QOS_Info: constant String(1..2) := (G.S.Timelayer.Get_QOS_Sym,
						G.S.Minutelayer.Get_QOS_Sym);
	begin
		G.Screen_Idx           := G.Screen_Idx + 1;
		G.Screen(G.Screen_Idx) := (X => XI, Y => YI,
					Msg => SB.To_Bounded_String(QOS_Info),
					others => <>);
	end Add_QOS;

	procedure Add_AL(G: in out GUI; YI: in Pos_Y;
					Underline: in Underline_Info) is
		AL: constant DCF77_Alarm.Time_T := G.S.Alarm.Get_AL_Time;
		IU: Natural := G.Screen_Idx;
	begin
		-- “AL” Prefix (1)
		IU := IU + 1;
		G.Screen(IU) := (X => 80, Y => YI,
				Msg => SB.To_Bounded_String("A"),
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
			G.Screen(IU).Msg := SB.To_Bounded_String((
						if G.Numeric_Editing_Enabled
						then "Save  " else "Edit  "));
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

	procedure Add_Info_Ctr(G: in out GUI) is
		use type DCF77_Low_Level.Time;
	begin
		G.Add_Info(
		Title =>
			"CTRInfo",
		L1 =>
			-- EEEE - LL Fault
			Num_To_Str_L4(G.S.LL.Get_Fault) & " " &
			-- AAAA - Bitlayer Unidentified
			Num_To_Str_L4(G.S.Bitlayer.Get_Unidentified) & " " &
			-- CC   - Bitlayer Overflown (rare)
			Num_To_Str_L2(G.S.Bitlayer.Get_Overflown) & " " &
			-- DDD  - Bitlayer Delay
			Num_To_Str_L3(Natural(G.S.Bitlayer.Get_Delay / 1000)),
		L2 =>
			-- FFFF - Secondlayer Fault
			Num_To_Str_L4(G.S.Secondlayer.Get_Fault) & " " &
			-- GGGG - Minutelayer Fault
			Num_To_Str_L4(G.S.Minutelayer.Get_Fault) & "    " &
			-- HHH  - Light Sensor Reading
			Num_To_Str_L3(Natural(G.S.Light_Sensor_Reading))
		);
	end Add_Info_Ctr;

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

	procedure Add_Info_Bits(G: in out GUI) is
		L1, L2: String(1 .. 16) := (others => ' ');
	begin
		G.S.Bitlayer.Draw_Bits_State(L1, L2);
		G.Add_Info("Bit." & G.S.Bitlayer.Get_Input, L1, L2);
	end Add_Info_Bits;

	procedure Add_Info_Oszi(G: in out GUI) is
		L1, L2: String(1 .. 16) := (others => ' ');
	begin
		G.S.Bitlayer.Draw_Bits_Oszi(L1, L2);
		G.Add_Info("BitOszi", L1, L2);
	end Add_Info_Oszi;

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

	-- DDHHiissDDHHiiss
	-- FYYYYMM-DDHHiiss
	procedure Add_Info_Alarm(G: in out GUI) is
		Trace_Button_Down: constant TM :=
						G.S.Alarm.Get_Trace_Button_Down;
		Trace_Button_Up:   constant TM := G.S.Alarm.Get_Trace_Button_Up;
		Trace_Alarm_Fired: constant TM :=
						G.S.Alarm.Get_Trace_Alarm_Fired;
		L3: String(1 .. 16);
		L4: String(1 .. 16);
	begin
		L3(1  ..  2) := Num_To_Str_L2(Trace_Button_Down.D);
		L3(3  ..  4) := Num_To_Str_L2(Trace_Button_Down.H);
		L3(5  ..  6) := Num_To_Str_L2(Trace_Button_Down.I);
		L3(7  ..  8) := Num_To_Str_L2(Trace_Button_Down.S);
		L3(9  .. 10) := Num_To_Str_L2(Trace_Button_Up.D);
		L3(11 .. 12) := Num_To_Str_L2(Trace_Button_Up.H);
		L3(13 .. 14) := Num_To_Str_L2(Trace_Button_Up.I);
		L3(15 .. 16) := Num_To_Str_L2(Trace_Button_Up.S);

		L4(1)        := 'F';
		L4(2  ..  5) := Num_To_Str_L4(Trace_Alarm_Fired.Y);
		L4(6  ..  7) := Num_To_Str_L2(Trace_Alarm_Fired.M);
		L4(8)        := '-';
		L4(9  .. 10) := Num_To_Str_L2(Trace_Alarm_Fired.D);
		L4(11 .. 12) := Num_To_Str_L2(Trace_Alarm_Fired.H);
		L4(13 .. 14) := Num_To_Str_L2(Trace_Alarm_Fired.I);
		L4(15 .. 16) := Num_To_Str_L2(Trace_Alarm_Fired.s);

		G.Add_Info("Al[v^F]", L3, L4);
	end Add_Info_Alarm;

end DCF77_GUI;
