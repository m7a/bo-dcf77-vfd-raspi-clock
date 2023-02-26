with RP.Clock;
with HAL.SPI;
with HAL.UART;
with RP.GPIO.Interrupts;

with DCF77_Functions;
use  DCF77_Functions; -- Inc_Saturated

package body DCF77_Low_Level is

	-- Disable warnings about this for the entire file since we want to
	-- design the API in a way that dependency on Ctx can later be added
	-- for any of the procedures as necessary.
	pragma Warnings(Off, "formal parameter ""Ctx"" is not referenced");

	procedure Init(Ctx: in out LL) is
	begin
		-- Clocks and Timers
		RP.Clock.Initialize(Pico.XOSC_Frequency);
		RP.Clock.Enable(RP.Clock.ADC); -- ADC
		RP.Clock.Enable(RP.Clock.PERI); -- SPI
		RP.Device.Timer.Enable;

		-- Digital Inputs
		DCF.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		DCF.Enable_Interrupt(RP.GPIO.Rising_Edge);
		DCF.Enable_Interrupt(RP.GPIO.Falling_Edge);
		RP.GPIO.Interrupts.Attach_Handler(DCF, Handle_DCF_Interrupt'Access);
		Not_Ta_G.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Ta_L.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Ta_R.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);

		-- Analog Inputs
		Light.Configure(RP.GPIO.Input, RP.GPIO.Floating);
		Light.Configure(RP.GPIO.Analog);
		Ctx.ADC0_Light := RP.ADC.To_ADC_Channel(Light);
		RP.ADC.Configure(Ctx.ADC0_Light);

		-- Outputs
		Control_Or_Data.Configure(RP.GPIO.Output);
		Buzzer.Configure(RP.GPIO.Output);
		Alarm_LED.Configure(RP.GPIO.Output);

		-- SPI (https://pico-doc.synack.me/#spi)
		SCK.Configure   (RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		TX.Configure    (RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		RX.Configure    (RP.GPIO.Input,  RP.GPIO.Pull_Up,  RP.GPIO.SPI);
		Not_CS.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		-- if it matters, Arduino has DORD - data order lsb first?
		SPI_Port.Configure(Config => (
			Role      => RP.SPI.Master,
			Baud      => 1_000_000,            -- 1MHz, up to 50MHz!
			Data_Size => HAL.SPI.Data_Size_8b, -- not on Arduino
			Polarity  => RP.SPI.Active_Low,    -- clock idle at 1
			Phase     => RP.SPI.Rising_Edge,   -- sample on leading
			Blocking  => True,
			others    => <>                    -- Loopback = False
		));

		-- UART (https://pico-doc.synack.me/#uart)
		-- https://github.com/JeremyGrosser/pico_examples/blob/master/
		-- uart_echo/src/main.adb
		UTX.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.UART);
		URX.Configure(RP.GPIO.Input,  RP.GPIO.Floating, RP.GPIO.UART);
		UART_Port.Configure; -- use default 115200 8n1
	end Init;
	
	-- This parameter is present in the fixed API but currently unused.
	pragma Warnings(Off, "formal parameter ""Pin"" is not referenced");
	procedure Handle_DCF_Interrupt(Pin: RP.GPIO.GPIO_Pin;
					Trigger: RP.GPIO.Interrupt_Triggers) is
	pragma Warnings(On,  "formal parameter ""Pin"" is not referenced");
	begin
		case Trigger is
		when RP.GPIO.Rising_Edge =>
			-- Input voltage 0/1 transition means DCF77 0/1
			-- transition. Begin of a "high" signal
			if Interrupt_Start_Ticks = 0 then
				Interrupt_Start_Ticks := Time(RP.Timer.Clock);
			else
				Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
			end if;
		when RP.GPIO.Falling_Edge =>
			-- Input voltage 1/0 transition means DCF77 1/0
			-- transition. End of a "high" signal
			if Interrupt_Start_Ticks = 0 then
				Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
			else 
				if Interrupt_Out_Ticks /= 0 then
					Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
				end if;
				Interrupt_Out_Ticks := Time(RP.Timer.Clock) -
							Interrupt_Start_Ticks;
				Interrupt_Start_Ticks := 0;
			end if;
		when others => 
			Inc_Saturated(Interrupt_Fault, Interrupt_Fault_Max);
		end case;
	end Handle_DCF_Interrupt;

	function Get_Time_Micros(Ctx: in out LL) return Time is
							(Time(RP.Timer.Clock));

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

	function Read_Interrupt_Signal(Ctx: in out LL) return Time is
		Ret: constant Time := Interrupt_Out_Ticks;
	begin
		if Ret /= 0 then
			-- mark that the data was processed successfully
			Interrupt_Out_Ticks := 0;
		end if;
		return Ret;
	end Read_Interrupt_Signal;

	function Read_Green_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_G.Get);
	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_L.Get);
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_R.Get);

	function Read_Light_Sensor(Ctx: in out LL) return Light_Value is
		use type RP.ADC.Microvolts;
		Val: constant RP.ADC.Microvolts :=
					RP.ADC.Read_Microvolts(Ctx.ADC0_Light);
	begin
		-- TODO MAY WANT TO EMBED SOME TUNED CONVERSION HERE
		return Light_Value(RP.ADC.Microvolts'Min(
			RP.ADC.Microvolts(Light_Value'Last),
			Val * RP.ADC.Microvolts(Light_Value'Last) / 3_300_000));
	end Read_Light_Sensor;

	procedure Log(Ctx: in out LL; Msg: in String) is
		Data: HAL.UART.UART_Data_8b(Msg'Range)
						with Address => Msg'Address;
		Status: HAL.UART.UART_Status;
	begin
		UART_Port.Transmit(Data, Status);
	end Log;

end DCF77_Low_Level;
