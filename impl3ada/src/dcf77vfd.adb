with DCF77_Low_Level;
with DCF77_Display;
with DCF77_Ticker;
with DCF77_Types;
with DCF77_Bitlayer;
with DCF77_Secondlayer;

with DCF77_Functions; -- TODO z TMP
use  DCF77_Functions;
use  DCF77_Types; -- TODO z TMP
use  DCF77_Low_Level; -- TODO z TMP Access time easily
with DCF77_Offsets; -- TODO z TMP
use  DCF77_Offsets; -- TODO z TMP

use type DCF77_Secondlayer.Telegram_State;

procedure DCF77VFD is

	-- Hardware-specific
	LL:       aliased DCF77_Low_Level.LL;
	Disp:     aliased DCF77_Display.Disp;
	Ticker:   aliased DCF77_Ticker.Ticker;
	Bitlayer: aliased DCF77_Bitlayer.Bitlayer;

	Bitlayer_Reading: DCF77_Types.Reading;

	-- Software only
	Secondlayer:            aliased DCF77_Secondlayer.Secondlayer;
	Secondlayer_Telegram_1: DCF77_Secondlayer.Telegram;
	Secondlayer_Telegram_2: DCF77_Secondlayer.Telegram;

	-- TMP BEGIN EXPERIMENTAL STUFF
	Date:      String := "20YY-MM-DD";
	Prefix:    String := "HH:ii";
	Date_B:    DCF77_Display.SB.Bounded_String;
	Prefix_B:  DCF77_Display.SB.Bounded_String;
	Second:    Natural := 0;
	New_Time:  DCF77_Low_Level.Time;
	Last_Time: DCF77_Low_Level.Time := 0;
	Last_Reading: DCF77_Types.Reading := No_Update;

	-- TODO x Quick and dirty triple conversion...
	function BCD_To_Char(Offset: in Natural; Length: in Natural)
							return Character is
		function Decode_BCD(Data: in DCF77_Secondlayer.Bits)
							return Natural is
			Rslt: Natural := 0;
		begin
			for Val of Data loop
				Rslt := Rslt * 2;
				if Val = Bit_1 then
					Rslt := Rslt + 1;
				end if;
			end loop;
			return Rslt;
		end Decode_BCD;
	begin
		case Decode_BCD(Secondlayer_Telegram_1.Value(Offset ..
								Length)) is
		when 0      => return '0';
		when 1      => return '1';
		when 2      => return '2';
		when 3      => return '3';
		when 4      => return '4';
		when 5      => return '5';
		when 6      => return '6';
		when 7      => return '7';
		when 8      => return '8';
		when 9      => return '9';
		when others => return '?';
		end case;
	end BCD_To_Char;
	-- END EXPERIMENTAL STUFF

begin
	LL.Init;
	Disp.Init(LL'Access);
	Ticker.Init(LL'Access);
	Bitlayer.Init(LL'Access);
	Secondlayer.Init;

	LL.Log("BEFORE CTR=28");

	Disp.Update((
		1 => (X => 16, Y => 16, F => DCF77_Display.Small,
		Msg => DCF77_Display.SB.To_Bounded_String("INIT CTR=27"))
	));

	loop
		LL.Debug_Dump_Interrupt_Info;
		--LL.Log("Hello");

		Bitlayer_Reading := Bitlayer.Update;
		Secondlayer.Process(Bitlayer_Reading,
				Secondlayer_Telegram_1, Secondlayer_Telegram_2);

		-- BEGIN EXPERIMENTAL STUFF --
		New_Time := LL.Get_Time_Micros;
		if New_Time > Last_Time + 1_000_000 then
			Last_Time := New_Time;
			Inc_Saturated(Second, 1000);
		end if;

		if Secondlayer_Telegram_1.Valid =
					DCF77_Secondlayer.Valid_60 then
			Date(Date'First + 2) := BCD_To_Char(
					Offset_Year_Tens, Length_Year_Tens);
			Date(Date'First + 3) := BCD_To_Char(
					Offset_Year_Ones, Length_Year_Ones);
			Date(Date'First + 5) := BCD_To_Char(
					Offset_Month_Tens, Length_Month_Tens);
			Date(Date'First + 6) := BCD_To_Char(
					Offset_Month_Ones, Length_Month_Ones);
			Date(Date'First + 8) := BCD_To_Char(
					Offset_Day_Tens, Length_Day_Tens);
			Date(Date'First + 9) := BCD_To_Char(
					Offset_Day_Ones, Length_Day_Ones);
			Prefix(Prefix'First + 0) := BCD_To_Char(
					Offset_Hour_Tens, Length_Hour_Tens);
			Prefix(Prefix'First + 1) := BCD_To_Char(
					Offset_Hour_Ones, Length_Hour_Ones);
			Prefix(Prefix'First + 3) := BCD_To_Char(
					Offset_Minute_Tens, Length_Minute_Tens);
			Prefix(Prefix'First + 4) := BCD_To_Char(
					Offset_Minute_Ones, Length_Minute_Ones);
			Date_B   := DCF77_Display.SB.To_Bounded_String(Date);
			Prefix_B := DCF77_Display.SB.To_Bounded_String(Prefix);
			Second   := 0;
			Secondlayer_Telegram_1.Valid :=
						DCF77_Secondlayer.Invalid;
		end if;
		if Bitlayer_Reading /= No_Update then
			Last_Reading := Bitlayer_Reading;
		end if;

		Disp.Update((
			1 => (X => 0, Y => 0, F => DCF77_Display.Small,
							Msg => Date_B),
			2 => (X => 80, Y => 0, F => DCF77_Display.Small,
				Msg => DCF77_Display.SB.To_Bounded_String(
				Time'Image(Ticker.Get_Delay / 1000))),
			3 => (X => 80, Y => 16, F => DCF77_Display.Small,
				Msg => DCF77_Display.SB.To_Bounded_String(
				Natural'Image(Second))),
			4 => (X => 80, Y => 32, F => DCF77_Display.Small,
				Msg => DCF77_Display.SB.To_Bounded_String(
				Natural'Image(Bitlayer.Get_Unidentified))),
			5 => (X => 0, Y => 48, F => DCF77_Display.Small,
				Msg => DCF77_Display.SB.To_Bounded_String(
				DCF77_Types.Reading'Image(Last_Reading) &
				" FL" & Natural'Image(LL.Get_Fault))),
			6 => (X => 0, Y => 16, F => DCF77_Display.Large,
				Msg => Prefix_B)
		));
		-- END EXPERIMENTAL STUFF --

		Ticker.Tick; -- last operation before next iteration
	end loop;

end DCF77VFD;
