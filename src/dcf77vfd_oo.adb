with DCF77_Display;
with DCF77_Ticker;
with DCF77_Types;
with DCF77_Bitlayer;
with DCF77_ST_Layer_Shared;
with DCF77_Secondlayer;
with DCF77_Timelayer;
with DCF77_Low_Level;

with DCF77_Functions;
use  DCF77_Functions; -- x Num_To_Str_L2, L4

use type DCF77_Types.Reading;
use type DCF77_Low_Level.Time;

package body DCF77VFD_OO is

	procedure Main is
		LL:       constant DCF77_Low_Level.LLP := LLI'Access;
		Disp:     aliased DCF77_Display.Disp;
		Ticker:   aliased DCF77_Ticker.Ticker;
		Bitlayer: aliased DCF77_Bitlayer.Bitlayer;

		Bitlayer_Reading: DCF77_Types.Reading;

		-- Software only
		Secondlayer:            aliased DCF77_Secondlayer.Secondlayer;
		Secondlayer_Telegram_1: DCF77_ST_Layer_Shared.Telegram;
		Secondlayer_Telegram_2: DCF77_ST_Layer_Shared.Telegram;
		Timelayer:              aliased DCF77_Timelayer.Timelayer;

		Bitlayer_Has_New: Boolean;
		Datetime:         DCF77_Timelayer.TM;

		Date_S:       String := "20YYMMDD";
		Time_S:       String := "HH:ii:ss";
		Date_B:       DCF77_Display.SB.Bounded_String;
		Time_B:       DCF77_Display.SB.Bounded_String;
		Last_Reading: DCF77_Types.Reading := DCF77_Types.No_Update;

		function Describe_QOS(Q: in DCF77_Timelayer.QOS)
							return String is
		begin
			case Q is
			when DCF77_Timelayer.QOS1       => return "+1";
			when DCF77_Timelayer.QOS2       => return "+2";
			when DCF77_Timelayer.QOS3       => return "+3";
			when DCF77_Timelayer.QOS4       => return "o4";
			when DCF77_Timelayer.QOS5       => return "o5";
			when DCF77_Timelayer.QOS6       => return "o6";
			when DCF77_Timelayer.QOS7       => return "-7";
			when DCF77_Timelayer.QOS8       => return "-8";
			when DCF77_Timelayer.QOS9_ASYNC => return "-9";
			end case;
		end Describe_QOS;

	begin
		LL.Init;
		Disp.Init(LL);
		Ticker.Init(LL);
		Bitlayer.Init(LL);
		Secondlayer.Init;
		Timelayer.Init;

		LL.Log("BEFORE CTR=39");
		Disp.Update((1 => (X => 16, Y => 16, F => DCF77_Display.Small,
				Msg => DCF77_Display.SB.To_Bounded_String(
				"INIT CTR=39"))));

		loop
			LL.Debug_Dump_Interrupt_Info;

			Bitlayer_Reading := Bitlayer.Update;
			Secondlayer.Process(Bitlayer_Reading,
				Secondlayer_Telegram_1, Secondlayer_Telegram_2);

			Bitlayer_Has_New := Bitlayer_Reading /=
							DCF77_Types.No_Update;
			-- TODO HEAVY DRIFT WHEN RUN W/O SYNC! NEED TO ADJUST THIS SOMEHOW...
			-- In principle we have to adjust the bitlayer to query
			-- one earlier when the preceding output was No_Signal.
			-- Since that was after 11 iterations, next check after
			-- 9 iterations to sum up to 20 i.e. 2000ms or something.
			if Bitlayer_Has_New then
				Timelayer.Process(True, -- Bitlayer_Has_New,
							Secondlayer_Telegram_1,
							Secondlayer_Telegram_2);
			end if;

			Datetime := Timelayer.Get_Current;
			--Date_S := Nat_To_S(Datetime.Y, 4) & "-" &
			--		Nat_To_S(Datetime.M, 2) & "-" &
			--		Nat_To_S(Datetime.D, 2);
			Date_S := Num_To_Str_L4(Datetime.Y) &
					Num_To_Str_L2(Datetime.M) &
					Num_To_Str_L2(Datetime.D);
			Time_S := Num_To_Str_L2(Datetime.H) & ":" &
					Num_To_Str_L2(Datetime.I) & ":" &
					Num_To_Str_L2(Datetime.S);
			Date_B := DCF77_Display.SB.To_Bounded_String(Date_S);
			Time_B := DCF77_Display.SB.To_Bounded_String(Time_S);

			if Bitlayer_Has_New then
				Last_Reading := Bitlayer_Reading;
			end if;

			Disp.Update((
				1 => (X => 0, Y => 0, F => DCF77_Display.Small,
								Msg => Date_B),
				2 => (X => 68, Y => 0, F => DCF77_Display.Small,
					Msg => DCF77_Display.SB.To_Bounded_String(
					DCF77_Low_Level.Time'Image(
					Ticker.Get_Delay / 1000))),
				3 => (X => 100, Y => 0, F => DCF77_Display.Small,
					Msg => DCF77_Display.SB.To_Bounded_String(
					Describe_QOS(Timelayer.Get_Quality_Of_Service))),
				4 => (X => 80, Y => 32, F => DCF77_Display.Small,
					Msg => DCF77_Display.SB.To_Bounded_String(
					Natural'Image(Bitlayer.Get_Unidentified))),
				5 => (X => 0, Y => 48, F => DCF77_Display.Small,
					Msg => DCF77_Display.SB.To_Bounded_String(
					DCF77_Types.Reading'Image(Last_Reading) &
					" FL" & Natural'Image(LL.Get_Fault))),
				6 => (X => 0, Y => 16, F => DCF77_Display.Large,
					Msg => Time_B)
			));

			Ticker.Tick; -- last operation before next iteration
		end loop;
	end Main;

end DCF77VFD_OO;
