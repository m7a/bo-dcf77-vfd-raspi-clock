with Pico;
with RP.ADC;
with RP.GPIO;
with RP.Timer;
with RP.Device;
with RP.SPI;
with RP.UART;

-- with System.Atomic_Operations;
with Atomic.Unsigned_32;

with DCF77_Types;
use  DCF77_Types;

package DCF77_Low_Level is

	type SPI_Display_Mode is (Control, Data);

	type Time        is new RP.Timer.Time;
	type Light_Value is new Integer range 0 .. 100;
	type Bytes       is array (Natural range <>) of U8;

	type LL is tagged limited private;
	type LLP is access all LL;

	procedure Init(Ctx: in out LL);

	function Get_Time_Micros(Ctx: in out LL) return Time;
	procedure Delay_Micros(Ctx: in out LL; DT: in Time);
	procedure Delay_Until(Ctx: in out LL; T: in Time);

	function Read_Interrupt_Signal_And_Clear(Ctx: in out LL) return U32;

	-- Returns True when Button is held down
	function Read_Green_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Alarm_Switch_Is_Enabled(Ctx: in out LL) return Boolean;

	-- Returns "percentage" scale value
	function Read_Light_Sensor(Ctx: in out LL) return Light_Value;

	procedure Set_Buzzer_Enabled(Ctx: in out LL; Enabled: in Boolean);
	procedure Set_Alarm_LED_Enabled(Ctx: in out LL; Enabled: in Boolean);

	-- New SPI display transfer API to replace all the old approaches
	procedure SPI_Display_Transfer_Reversed(Ctx: in out LL;
			Send_Value: in Bytes; Mode: in SPI_Display_Mode);

	function Get_Fault(Ctx: in out LL) return Natural;

	procedure Log(Ctx: in out LL; Msg: in String);

	procedure Debug_Dump_Interrupt_Info(Ctx: in out LL);

private

	procedure Handle_DCF_Interrupt;

	type LL is tagged limited record
		ADC0_Light: RP.ADC.ADC_Channel;
	end record;

	--type Atomic_U32 is new U32 with Atomic;
	--package Exchange is new Atomic_Operations.Exchange(Atomic_Type =>
	--							Atomic_U32);

	-- Digital Inputs
	DCF:             RP.GPIO.GPIO_Point renames Pico.GP22;
	Not_Ta_G:        RP.GPIO.GPIO_Point renames Pico.GP15;
	Not_Ta_L:        RP.GPIO.GPIO_Point renames Pico.GP21;
	Not_Ta_R:        RP.GPIO.GPIO_Point renames Pico.GP20;
	Not_Sa_Al:       RP.GPIO.GPIO_Point renames Pico.GP19;

	-- Analog Inputs
	Light:           RP.GPIO.GPIO_Point renames Pico.GP26;

	-- Outputs
	Control_Or_Data: RP.GPIO.GPIO_Point renames Pico.GP14;
	Buzzer:          RP.GPIO.GPIO_Point renames Pico.GP3;
	Alarm_LED:       RP.GPIO.GPIO_Point renames Pico.GP28;

	-- SPI
	SCK:             RP.GPIO.GPIO_Point renames Pico.GP10;
	TX:              RP.GPIO.GPIO_Point renames Pico.GP11;
	RX:              RP.GPIO.GPIO_Point renames Pico.GP12;
	Not_CS:          RP.GPIO.GPIO_Point renames Pico.GP13;
	SPI_Port:        RP.SPI.SPI_Port    renames RP.Device.SPI_1;

	-- UART
	UTX:             RP.GPIO.GPIO_Point renames Pico.GP0;
	URX:             RP.GPIO.GPIO_Point renames Pico.GP1;
	UART_Port:       RP.UART.UART_Port  renames RP.Device.UART_0;

	-- Static Variables for Interrupt Counter.
	-- Cannot be part of the record for lifecycle reasons
	-- ISR procedure may run even if context already destroyed!
	Interrupt_Fault_Max: constant Natural := 9999;
	Interrupt_Fault: Natural := 0;
	--Interrupt_Value: Atomic_U32 := 1;
	Interrupt_Value: aliased Atomic.Unsigned_32.Instance :=
						Atomic.Unsigned_32.Init(1);

	function Get_Fault(Ctx: in out LL) return Natural is (Interrupt_Fault);

end DCF77_Low_Level;
