with RP.Clock;
with HAL.SPI;
with HAL.UART;
with RP_Interrupts;
with RP2040_SVD.TIMER;
with RP2040_SVD.Interrupts;
with Interfaces;
use  Interfaces;

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
		use type HAL.Uint32; -- import + operator
		-- Inverted by antenna design
		Val: constant Boolean := not DCF.Get;
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
				Interrupt_Out_Ticks := Time(RP.Timer.Clock) -
							Interrupt_Start_Ticks;
				Interrupt_Pending_Read := True;
			end if;
			-- TODO z COULD ADD SOME SORT OF "ALL ZERO" RECOGNIZTION?
			--        => better "No_Signal" recogniztion than a
			--        timeout?
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

	-- The low level functions (chip) only support msb first, but the
	-- display module requires lsb first. These conversion function should
	-- do the job clearly and efficiently.
	function Reverse_Bits_8(V: in U8) return U8 is (
		Shift_Left (V and 16#01#, 7) or Shift_Right(V and 16#80#, 7) or
		Shift_Left (V and 16#02#, 5) or Shift_Right(V and 16#40#, 5) or
		Shift_Left (V and 16#04#, 3) or Shift_Right(V and 16#20#, 3) or
		Shift_Left (V and 16#08#, 1) or Shift_Right(V and 16#10#, 1));

	function Reverse_Bits_16(V: in U16) return U16 is (
		U16(Reverse_Bits_8(U8(Shift_Right(V and 16#ff00#, 8)))) or
		Shift_Left(U16(Reverse_Bits_8(U8(V and 16#00ff#))), 8));

	function Reverse_Bits_32(V: in U32) return U32 is (
		U32(Reverse_Bits_16(U16(Shift_Right(V and 16#ffff0000#, 16))))
		or
		Shift_Left(U32(Reverse_Bits_16(U16(V and 16#0000ffff#))), 16));

	procedure SPI_Display_Transfer_Gen(Ctx: in out LL; Send_Value: in Num;
						Mode: in SPI_Display_Mode) is
		Status: HAL.SPI.SPI_Status;
		Value_Rev: Num := Reverse_Bits(Send_Value);
		Data_Out: HAL.SPI.SPI_Data_8b(1 .. Len);
		for Data_Out'Address use Value_Rev'Address;
	begin
		case Mode is
		when Control => Control_Or_Data.Set;
		when Data    => Control_Or_Data.Clear;
		end case;
		SPI_Port.Transmit(Data_Out, Status);
	end SPI_Display_Transfer_Gen;

	procedure SPI_Display_Transfer_U8 is
		new SPI_Display_Transfer_Gen(U8, 1, Reverse_Bits_8'Access);
	procedure SPI_Display_Transfer(Ctx: in out LL; Send_Value: in U8;
				Mode: in SPI_Display_Mode)
				renames SPI_Display_Transfer_U8;
	procedure SPI_Display_Transfer_U16 is
		new SPI_Display_Transfer_Gen(U16, 2, Reverse_Bits_16'Access);
	procedure SPI_Display_Transfer(Ctx: in out LL; Send_Value: in U16;
				Mode: in SPI_Display_Mode)
				renames SPI_Display_Transfer_U16;
	procedure SPI_Display_Transfer_U32 is
		new SPI_Display_Transfer_Gen(U32, 4, Reverse_Bits_32'Access);
	procedure SPI_Display_Transfer(Ctx: in out LL; Send_Value: in U32;
				Mode: in SPI_Display_Mode)
				renames SPI_Display_Transfer_U32;

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
