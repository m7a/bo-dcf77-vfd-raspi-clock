with RP.Clock;
with HAL.SPI;
with HAL.UART;
with RP_Interrupts;
with RP2040_SVD.TIMER;
with RP2040_SVD.Interrupts;

with DCF77_Functions;
use  DCF77_Functions; -- Inc_Saturated

package body DCF77_Low_Level is

	-- Disable warnings about this for the entire file since we want to
	-- design the API in a way that dependency on Ctx can later be added
	-- for any of the procedures as necessary.
	pragma Warnings(Off, "formal parameter ""Ctx"" is not referenced");

	procedure Init(Ctx: in out LL) is
		use type HAL.Uint32; -- import + operator
	begin
		-- Clocks and Timers
		RP.Clock.Initialize(Pico.XOSC_Frequency);
		RP.Clock.Enable(RP.Clock.ADC); -- ADC
		RP.Clock.Enable(RP.Clock.PERI); -- SPI
		RP.Device.Timer.Enable;

		-- Digital Inputs
		DCF.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Ta_G.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Ta_L.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Ta_R.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Sa_Al.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);

		-- Analog Inputs
		Light.Configure(RP.GPIO.Analog);
		Ctx.ADC0_Light := RP.ADC.To_ADC_Channel(Light);
		RP.ADC.Configure(Ctx.ADC0_Light);
		RP.ADC.Enable;

		-- Outputs
		Control_Or_Data.Configure(RP.GPIO.Output);
		Buzzer.Configure(RP.GPIO.Output);
		Alarm_LED.Configure(RP.GPIO.Output);

		-- SPI (https://pico-doc.synack.me/#spi)
		SCK.Configure   (RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		TX.Configure    (RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		RX.Configure    (RP.GPIO.Input,  RP.GPIO.Pull_Up,  RP.GPIO.SPI);
		Not_CS.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		-- NB: Arduino has DORD -- data order lsb first, but the RPI2040
		--     does not seem to support it. We need to change the order
		--     of bytes on the software side!
		SPI_Port.Configure(Config => (
			Role      => RP.SPI.Master,        -- min 1MHz max 50MHz
			Baud      => 10_000_000,           -- here: 10MHz SEL
			Data_Size => HAL.SPI.Data_Size_8b, -- not on Arduino
			Polarity  => RP.SPI.Active_Low,    -- clock idle at 1
			Phase     => RP.SPI.Rising_Edge,   -- sample on leading
			Blocking  => True,                 -- C/D aligned
			others    => <>                    -- Loopback = False
		));

		-- Initial delay of 100ms before starting to read DCF77 data
		-- from antenna
		RP2040_SVD.TIMER.TIMER_Periph.ALARM1 := RP2040_SVD.TIMER.
						Timer_Periph.TIMERAWL + 100_000;
		RP2040_SVD.TIMER.TIMER_Periph.INTE.ALARM_1 := True;
		RP_Interrupts.Attach_Handler(
			Handler => Handle_DCF_Interrupt'Access,
			Id      => RP2040_SVD.Interrupts.TIMER_IRQ_1_Interrupt,
			Prio    => RP_Interrupts.Interrupt_Priority'First
		);

		-- UART (https://pico-doc.synack.me/#uart)
		-- https://github.com/JeremyGrosser/pico_examples/blob/master/
		-- uart_echo/src/main.adb
		UTX.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.UART);
		URX.Configure(RP.GPIO.Input,  RP.GPIO.Floating, RP.GPIO.UART);
		UART_Port.Configure; -- use default 115200 8n1
	end Init;
	
	procedure Handle_DCF_Interrupt is
		use type HAL.UInt32; -- import + operator
		-- Inverted by antenna design
		Val: constant Boolean := not DCF.Get;
		DT: Time;
	begin
		-- ack and trigger again in 7ms
		RP2040_SVD.TIMER.TIMER_Periph.INTR.ALARM_1 := True;
		RP2040_SVD.TIMER.TIMER_Periph.ALARM1 :=
				RP2040_SVD.TIMER.TIMER_Periph.ALARM1 + 7_000;
		RP2040_SVD.TIMER.TIMER_Periph.INTE.ALARM_1 := True;

		if Val then
			if Interrupt_Start_Ticks = 0 then
				Interrupt_Start_Ticks := Time(RP.Timer.Clock);
			elsif Interrupt_Pending_Read then
				Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
				Interrupt_Pending_Read := False;
				Interrupt_Start_Ticks  := Time(RP.Timer.Clock);
			end if;
		else
			if Interrupt_Start_Ticks /= 0 and
						not Interrupt_Pending_Read then
				DT := Time(RP.Timer.Clock) -
							Interrupt_Start_Ticks;
				-- Act like we never saw the “1” for very
				-- short spikes
				if DT < 10_000 then
					Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
					Interrupt_Start_Ticks := 0;
				-- give 2nd chance to complete the signal if
				-- it appears too short but there were
				-- definitely multiple “1” readings
				elsif DT < 30_000 then
					Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
				else
					Interrupt_Out_Ticks    := DT;
					Interrupt_Pending_Read := True;
				end if;
			end if;
		end if;
	end Handle_DCF_Interrupt;

	function Get_Time_Micros(Ctx: in out LL) return Time is
							(Time(RP.Timer.Clock));

	procedure Delay_Micros(Ctx: in out LL; DT: in Time) is
		use type RP.Timer.Time;
	begin
		RP.Timer.Busy_Wait_Until(RP.Timer.Clock + RP.Timer.Time(DT));
	end Delay_Micros;

	procedure Delay_Until(Ctx: in out LL; T: in Time) is
	begin
		RP.Timer.Busy_Wait_Until(RP.Timer.Time(T));
	end Delay_Until;

	procedure Set_Buzzer_Enabled(Ctx: in out LL; Enabled: in Boolean) is
	begin
		if Enabled then
			Buzzer.Set;
		else
			Buzzer.Clear;
		end if;
	end Set_Buzzer_Enabled;

	procedure Set_Alarm_LED_Enabled(Ctx: in out LL; Enabled: in Boolean) is
	begin
		if Enabled then
			Alarm_LED.Set;
		else
			Alarm_LED.Clear;
		end if;
	end Set_Alarm_LED_Enabled;

	function Read_Interrupt_Signal(Ctx: in out LL;
				Signal_Length: out Time; Signal_Begin: out Time)
				return Boolean is
	begin
		if Interrupt_Pending_Read then
			Signal_Length          := Interrupt_Out_Ticks;
			Signal_Begin           := Interrupt_Start_Ticks;
			Interrupt_Start_Ticks  := 0;
			Interrupt_Pending_Read := False;
			return True;
		else
			return False;
		end if;
	end Read_Interrupt_Signal;

	function Read_Green_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_G.Get);
	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_L.Get);
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_R.Get);
	function Read_Alarm_Switch_Is_Enabled(Ctx: in out LL) return Boolean is
							(not Not_Sa_Al.Get);

	function Read_Light_Sensor(Ctx: in out LL) return Light_Value is
		use type RP.ADC.Microvolts;
		Val: constant RP.ADC.Microvolts :=
					RP.ADC.Read_Microvolts(Ctx.ADC0_Light);
	begin
		-- REM: Due to the hardware implementation the logic signal
		--      is inverted.
		-- REM: Actual hardware limit is 82. Should maybe adjust to that
		--      s.t. 100% display brightness can be achieved in practice
		--      (it could in fact be related to that we measure 3.1V max
		--      on the ADC VREF...)
		return Light_Value'Last - Light_Value(RP.ADC.Microvolts'Min(
			RP.ADC.Microvolts(Light_Value'Last),
			Val * RP.ADC.Microvolts(Light_Value'Last) / 3_300_000));
	end Read_Light_Sensor;

	procedure SPI_Display_Transfer_Reversed(Ctx: in out LL;
						Send_Value: in Bytes;
						Mode: in SPI_Display_Mode) is
		Status: HAL.SPI.SPI_Status;
		Data_Out: HAL.SPI.SPI_Data_8b(1 .. Send_Value'Length);
		for Data_Out'Address use Send_Value'Address;
	begin
		case Mode is
		when Control => Control_Or_Data.Set;
		when Data    => Control_Or_Data.Clear;
		end case;
		SPI_Port.Transmit(Data_Out, Status);
	end SPI_Display_Transfer_Reversed;

	procedure Log(Ctx: in out LL; Msg: in String) is
		Msg_Cpy: constant String := Msg & Character'Val(16#0d#) &
							Character'Val(16#0a#);
		Data: HAL.UART.UART_Data_8b(Msg_Cpy'Range);
		for Data'Address use Msg_Cpy'Address;
		Status: HAL.UART.UART_Status; -- discard
	begin
		UART_Port.Transmit(Data, Status, 60);
	end Log;

	procedure Debug_Dump_Interrupt_Info(Ctx: in out LL) is
	begin
		Ctx.Log("IINFO ot=" & Time'Image(Interrupt_Out_Ticks) &
			" st=" & Time'Image(Interrupt_Start_Ticks) &
			" pd=" & Boolean'Image(Interrupt_Pending_Read) &
			" dv=" & Boolean'Image(DCF.Get));
	end Debug_Dump_Interrupt_Info;

end DCF77_Low_Level;
