with RP.Clock;
with HAL.UART;

with RP_Interrupts;
with RP2040_SVD.TIMER; -- TIMER_Periph
with RP2040_SVD.Interrupts;

use HAL; -- UInt32 test

package body TIMERTEST_OO is

	procedure Log(Msg: in String) is
		Msg_Cpy: constant String := Msg & Character'Val(16#0d#) &
							Character'Val(16#0a#);
		Data: HAL.UART.UART_Data_8b(Msg_Cpy'Range);
		for Data'Address use Msg_Cpy'Address;
		Status: HAL.UART.UART_Status; -- discard
	begin
		UART_Port.Transmit(Data, Status, 60);
	end Log;

	procedure Delay_Micros(DT: in Time) is
		use type RP.Timer.Time;
	begin
		RP.Timer.Busy_Wait_Until(RP.Timer.Clock + RP.Timer.Time(DT));
	end Delay_Micros;

	procedure Handle_DCF_Interrupt is
 	begin
		-- ack and trigger again in 7ms
		RP2040_SVD.TIMER.TIMER_Periph.INTR.ALARM_1 := True;
		RP2040_SVD.TIMER.TIMER_Periph.ALARM1 :=
			RP2040_SVD.TIMER.TIMER_Periph.ALARM1 + 7_000;
		RP2040_SVD.TIMER.TIMER_Periph.INTE.ALARM_1 := True;
		Ctr := Ctr + 1;
	end Handle_DCF_Interrupt;

	procedure Main is
		Sctr: Natural := 0;
		Start: Uint32;
		On: Boolean := False;
	begin
		-- Clocks and Timers
		RP.Clock.Initialize(Pico.XOSC_Frequency);
		RP.Clock.Enable(RP.Clock.PERI); -- SPI
		RP.Device.Timer.Enable;

		-- Digital Input
		DCF.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);

		-- LED
		LED.Configure(RP.GPIO.Output);

		-- UART
		UTX.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.UART);
		URX.Configure(RP.GPIO.Input,  RP.GPIO.Floating, RP.GPIO.UART);
		UART_Port.Configure; -- use default 115200 8n1

		-- Interrupts
		-- start delay of 100ms
		Start := RP2040_SVD.TIMER.Timer_Periph.TIMERAWL;
		RP2040_SVD.TIMER.TIMER_Periph.ALARM1 := Start + 100_000;
		RP2040_SVD.TIMER.TIMER_Periph.INTE.ALARM_1 := True;
		RP_Interrupts.Attach_Handler(
			Handler => Handle_DCF_Interrupt'Access,
			Id      => RP2040_SVD.Interrupts.TIMER_IRQ_1_Interrupt,
			Prio    => RP_Interrupts.Interrupt_Priority'First
		);

		loop
			if On then
				LED.Set;
			else
				LED.Clear;
			end if;
			Log("main ver=6 ictr=" & Natural'Image(Ctr) & " sctr=" &
					Natural'Image(Sctr) & " start=" &
					UInt32'Image(Start));
			Delay_Micros(300_000); -- wait 300ms then print
			On   := not On;
			Sctr := Sctr + 1;
		end loop;
	end Main;

end TIMERTEST_OO;
