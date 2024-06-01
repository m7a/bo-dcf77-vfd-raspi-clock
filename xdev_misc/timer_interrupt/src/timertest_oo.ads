with Pico;
with RP.GPIO;
with RP.Timer;
with RP.Device;
with RP.UART;

package TIMERTEST_OO is

	procedure Main;

private

	type Time is new RP.Timer.Time;

	-- Digital Inputs
	DCF:             RP.GPIO.GPIO_Point renames Pico.GP22;

	-- LED
	LED:             RP.GPIO.GPIO_Point renames Pico.LED;

	-- UART
	UTX:             RP.GPIO.GPIO_Point renames Pico.GP0;
	URX:             RP.GPIO.GPIO_Point renames Pico.GP1;
	UART_Port:       RP.UART.UART_Port  renames RP.Device.UART_0;

	Ctr:             Natural := 0; pragma Volatile(Ctr);

end TIMERTEST_OO;
