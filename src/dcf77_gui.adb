with DCF77_Functions;
use  DCF77_Functions; -- x Num_To_Str_L2, L4
with DCF77_Types;
with DCF77_Low_Level;

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
				1 => (X => 0, Y => 0, F => Small, Msg => Date_B),
				2 => (X => 68, Y => 0, F => Small,
					Msg => SB.To_Bounded_String(
					DCF77_Low_Level.Time'Image(
					G.S.Ticker.Get_Delay / 1000))),
				3 => (X => 100, Y => 0, F => Small,
					Msg => Describe_QOS(G.S.Timelayer.Get_Quality_Of_Service)),
				4 => (X => 80, Y => 32, F => Small,
					Msg => SB.To_Bounded_String(
					Natural'Image(G.S.Bitlayer.Get_Unidentified))),
				5 => (X => 0, Y => 48, F => Small,
					Msg => SB.To_Bounded_String(
					DCF77_Types.Reading'Image(Last_Reading) &
					" FL" & Natural'Image(G.S.LL.Get_Fault))),
				6 => (X => 0, Y => 16, F => Large,
					Msg => Time_B)
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
				"INIT CTR=39"))));
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

		-- Init Screen Displays

		G.Screen_Display(WIDX_Date).X := 0;
		G.Screen_Display(WIDX_Date).Y := 0;
		G.Screen_Display(WIDX_Date).F := Small;
		G.Screen_Display(WIDX_AL) := (X => 48, Y => 48, F => Small,
					Msg => SB.To_Bounded_String("      "));
		G.Screen_Display(WIDX_QOS) := (X => 0, Y => 48, F => Small,
					Msg => SB.To_Bounded_String("-9"));
		G.Screen_Display(WIDX_Time).X := 0;
		G.Screen_Display(WIDX_Time).Y := 16;
		G.Screen_Display(WIDX_Time).F := Large;
		Format_Datetime(DCF77_Timelayer.Time_Of_Compilation,
					G.Screen_Display(WIDX_Date).Msg,
					G.Screen_Display(WIDX_Time).Msg);

		G.Screen_Edit := (
			WIDX_Date    => G.Screen_Display(WIDX_Date),
			WIDX_AL      => (X => 48, Y => 32, F => Small,
					Msg => SB.To_Bounded_String("AL00:00")),
			WIDX_QOS     => (X =>  0, Y => 32, F => Small,
					Msg => SB.To_Bounded_String("-9")),
			WIDX_Time    => (X =>  0, Y => 16, F => Small,
					Msg => G.Screen_Edit(WIDX_Time).Msg),
			WIDX_M_Next  => (X =>  0, Y => 48, F => Small,
					Msg => SB.To_Bounded_String("Next")),
			WIDX_M_Minus => (X => 40, Y => 48, F => Small,
					Msg => SB.To_Bounded_String("<")),
			WIDX_M_Plus  => (X => 56, Y => 48, F => Small,
					Msg => SB.To_Bounded_String(">")),
			WIDX_M_AL    => (X => 64, Y => 48, F => Small,
					Msg => SB.To_Bounded_String("AL"))
		);

		-- TODO X FOR OPTIONS AND INFO SIMILAR...
	end Init;

	procedure Format_Datetime(T: in DCF77_Timelayer.TM;
					Date, Time: out SB.Bounded_String) is
		Date_S: constant String := Num_To_Str_L4(T.Y) & "-" &
				Num_To_Str_L2(T.M) & "-" & Num_To_Str_L2(T.D);
	begin
		Date := SB.To_Bounded_String(Date_S);
		Format_Time(T, Time);
	end Format_Datetime;

	procedure Format_Time(T: in DCF77_Timelayer.TM;
					Time: out SB.Bounded_String) is
		Time_S: constant String := Num_To_Str_L2(T.H) & ":" &
				Num_To_Str_L2(T.I) & ":" & Num_To_Str_L2(T.S);
	begin
		Time := SB.To_Bounded_String(Time_S);
	end Format_Time;

	procedure Process(G: in out GUI) is
	begin
		G.Update_Display;
		-- TODO COMPUTE BEFORE/AFTER/INSIDE? HOW TO ALIGN CHANGES AND DISPLAY UPDATES? / CSTAT DO IMPL HERE / MAYBE RETHINK THE GUI STORAGE DESIGN... CSTAT
	end Process;

	procedure Update_Display(G: in out GUI) is
	begin
		case G.A is
		when Select_Display =>
			Format_Datetime(G.S.Datetime,
					G.Screen_Display(WIDX_Date).Msg,
					G.Screen_Display(WIDX_Time).Msg);
			G.Screen_Display(WIDX_AL).Msg :=
				(if G.S.Alarm.Is_Alarm_Enabled then
				Describe_AL(G) else SB.To_Bounded_String(""));
			G.Screen_Display(WIDX_QOS).Msg := Describe_QOS(
					G.S.Timelayer.Get_Quality_Of_Service);
			G.S.Disp.Update(G.Screen_Display);
		when Select_Alarm | Select_AL_H | Select_AL_I |
				Select_Datetime | Select_DT_H | Select_DT_I |
				Select_DT_S | Select_DT_YH | Select_DT_Y |
				Select_DT_M | Select_DT_D =>
			Format_Datetime(G.S.Datetime,
					G.Screen_Edit(WIDX_Date).Msg,
					G.Screen_Edit(WIDX_Time).Msg);
			G.Screen_Display(WIDX_AL).Msg := Describe_AL(G);
			G.Screen_Display(WIDX_QOS).Msg := Describe_QOS(
					G.S.Timelayer.Get_Quality_Of_Service);
			G.S.Disp.Update(G.Screen_Edit);
		when Select_Options =>
			Format_Time(G.S.Datetime, G.Screen_Options(
								WIDX_Time).Msg);
			G.S.Disp.Update(G.Screen_Options);
		when Select_Info =>
			Format_Time(G.S.Datetime, G.Screen_Info(WIDX_Time).Msg);
			G.S.Disp.Update(G.Screen_Info);
		end case;
	end Update_Display;

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

	function Describe_AL(G: in GUI) return SB.Bounded_String is
		(SB.To_Bounded_String("AL" & G.S.Alarm.Get_AL_Time_Formatted));

end DCF77_GUI;
