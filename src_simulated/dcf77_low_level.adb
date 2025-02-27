with Ada.Text_IO;
with Ada.Assertions;
use  Ada.Assertions;

-- Simulation Text Protocol
-- ------------------------
--
-- * Integers in Requests may contain any number of additional leading spaces *
--
-- Call                         Reply
-- ---------------------------  -----------------------------------------
-- get_time_micros              ack,get_time_micros,<positive value>
-- ~~read_interrupt_signal~~    ~~ack,interrupt_signal,none~~
--                              ~~ack,interrupt_signal,<length>,<begin>~~
-- read_interrupt_signal_v2     ack,read_interrupt_signal_v2,<positive value>
-- green_button_is_down         ack,green_button_is_down,<0|1>
-- left_button_is_down          ack,left_button_is_down,<0|1>
-- right_button_is_down         ack,right_button_is_down,<0|1>
-- alarm_switch_is_enabled      ack,alarm_switch_is_enabled,<0|1>
-- read_light_sensor            ack,read_light_sensor,<0..100 percentage>
-- get_fault                    ack,get_fault,<positive value>
--
-- Cast (no reply)
-- ---------------------------
-- set_buzzer_enabled,<0|1>
-- set_alarm_led_enabled,<0|1>
-- spi08,<c|d>,<0..255>
-- ~~spi16,<c|d>,<0..65535>~~
-- ~~spi32,<c|d>,<0..4294967295>~~
-- log,<msg>

package body DCF77_Low_Level is

	-- We don't currently need it but may add access to CTX later...
	pragma Warnings(Off, "formal parameter ""Ctx"" is not referenced");

	procedure Init(Ctx: in out LL) is
	begin
		Ctx.A := 1;
	end Init;

	-- We cannot use this approach because then the ISR + Java-Times are not
	-- in sync. Could switch to this approach when we are back to simulating
	-- the ISR behaviour, too...
	-- ~~~
	-- Now:     constant Ada.Real_Time.Time := Ada.Real_Time.Clock;
	-- Seconds: Ada.Real_Time.Seconds_Cunt;
	-- Subsec:  Ada.Real_Time.Time_Span;
	-- Now.Split(Seconds, Subsec);
	-- return Time(Seconds) * 1_000_000 + Time(Subsec * 1_000_000);
	-- ~~~
	function Get_Time_Micros(Ctx: in out LL) return Time is
				(Time'Value(Call(Ctx, "get_time_micros")));

	-- If the stdin is closed, an End_Error is raised. Since the MCU
	-- implementation is not aware of the case that the program could end
	-- (it usually ends when power is shut off...) this seems to be the
	-- correct way to do it in the simulation.
	function Call(Ctx: in out LL; Query: in String) return String is
	begin
		Ada.Text_IO.Put_Line(Query);
		declare
			Reply: constant String := Ada.Text_IO.Get_Line;
		begin
			Assert(Reply'Length > Query'Length + 4,
						"Reply length too short");
			Assert(Reply(Reply'First + 4 ..
				Reply'First + 4 + Query'Length - 1) = Query,
				"Reply does not match query");
			return Reply(Reply'First + 4 + Query'Length + 1 ..
								Reply'Last);
		end;
	end Call; 

	procedure Delay_Micros(Ctx: in out LL; DT: in Time) is
	begin
		-- smaller delays than 1ms are only sensible on RT OS
		if DT > 1_000 then
			Ada.Text_IO.Flush;
			delay Duration(DT) / 1_000_000.0;
		end if;
	end Delay_Micros;

	procedure Delay_Until(Ctx: in out LL; T: in Time) is
		Now: constant Time := Ctx.Get_Time_Micros;
		DT:  constant Time := (if T > Now then (T - Now) else 0);
	begin
		Ctx.Delay_Micros(DT);
	end Delay_Until;

	-- TODO INCOMPATIBLE API - NEED TO EXTEND THE CODE TO RETURN THE BITS RATHER THAN “SIGNAL LENGTH”/”SIGNAL BEGIN” pairs...
	function Read_Interrupt_Signal_And_Clear(Ctx: in out LL) return U32 is
		Reply: constant U32 := U32'Value(
					Call(Ctx, "read_interrupt_signal_v2"));
	begin
		Assert(Reply /= 0,
			"ISR reply must have a leading 1 and thus cannot be 0");
		return Reply;
		--return (2#10000000000#);
	end Read_Interrupt_Signal_And_Clear;

	function Read_Green_Button_Is_Down(Ctx: in out LL) return Boolean is
				(Call_Bool(Ctx, "green_button_is_down"));

	-- Request = query_name
	-- Reply   = ack,query_name,0 or ack,query_name,1
	function Call_Bool(Ctx: in out LL; Query: in String) return Boolean is
		Reply: constant String := Call(Ctx, Query);
	begin
		Assert(Reply'Length = 1);
		case Reply(Reply'First) is
		when '0'    => return False;
		when '1'    => return True;
		when others => raise Assertion_Error with
					"Unknown bool value: <" & Reply & ">";
		end case;
	end Call_Bool;

	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean is
				(Call_Bool(Ctx, "left_button_is_down"));
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean is
				(Call_Bool(Ctx, "right_button_is_down"));
	function Read_Alarm_Switch_Is_Enabled(Ctx: in out LL) return Boolean is
				(Call_Bool(Ctx, "alarm_switch_is_enabled"));

	-- Returns "percentage" scale value
	function Read_Light_Sensor(Ctx: in out LL) return Light_Value is
			(Light_Value'Value(Call(Ctx, "read_light_sensor")));

	procedure Set_Buzzer_Enabled(Ctx: in out LL; Enabled: in Boolean) is
	begin
		Cast(Ctx, "set_buzzer_enabled," & Bool2Char(Enabled));
	end Set_Buzzer_Enabled;

	procedure Cast(Ctx: in out LL; Query: in String) is
	begin
		Ada.Text_IO.Put_Line(Query);
	end Cast;

	function Bool2Char(B: in Boolean) return Character is
						(if (B) then '1' else '0');

	procedure Set_Alarm_LED_Enabled(Ctx: in out LL; Enabled: in Boolean) is
	begin
		Cast(Ctx, "set_alarm_led_enabled," & Bool2Char(Enabled));
	end Set_Alarm_LED_Enabled;

	procedure SPI_Display_Transfer_Reversed(Ctx: in out LL;
			Send_Value: in Bytes; Mode: in SPI_Display_Mode) is
		-- For legacy reasons use spi8 and reverse bits...
		function Reverse_Bits_8(V: in U8) return U8 is (
			Shift_Left (V and 16#01#, 7) or
						Shift_Right(V and 16#80#, 7) or
			Shift_Left (V and 16#02#, 5) or
						Shift_Right(V and 16#40#, 5) or
			Shift_Left (V and 16#04#, 3) or
						Shift_Right(V and 16#20#, 3) or
			Shift_Left (V and 16#08#, 1) or
						Shift_Right(V and 16#10#, 1));
		MC: constant Character := Mode2Char(Mode);
	begin
		for I in Send_Value'Range loop
			Cast(Ctx, "spi08," & MC & "," &
				U8'Image(Reverse_Bits_8(Send_Value(I))));
		end loop;
	end SPI_Display_Transfer_Reversed;

	function Mode2Char(M: in SPI_Display_Mode) return Character is
	begin
		case M is
		when Control => return 'c';
		when Data    => return 'd';
		end case;
	end Mode2Char;

	function Get_Fault(Ctx: in out LL) return Natural is
					(Natural'Value(Call(Ctx, "get_fault")));

	procedure Log(Ctx: in out LL; Msg: in String) is
	begin
		Cast(Ctx, "log," & Msg);
	end Log;

	-- N_IMPL on Simulation
	procedure Debug_Dump_Interrupt_Info(Ctx: in out LL) is null;

end DCF77_Low_Level;
