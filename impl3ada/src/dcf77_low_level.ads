with Pico;
with RP.ADC;
with RP.GPIO;
with RP.Timer;
with RP.Device;
with RP.SPI;

package DCF77_Low_Level is

	type Time is new RP.Timer.Time;

	type Wheel_Selection is (
		Wheel_Default,
		Wheel_Timer_Enabe,
		Wheel_Timer_Configure,
		-- TODO JUST EXAMPLES. COULD DESIGN DIFFERENTLY...
		Wheel_Set_Time,
		Wheel_Set_Date,
		Wheel_Set_Year,
		Wheel_Display_Status,
		Wheel_Display_Debug,
		Wheel_Display_Version
	);

	type Light_Value is new Integer range 0 .. 100;

	type LL is tagged limited private;

	procedure Init(Ctx: in out LL);

	function Get_Time_Micros(Ctx: in out LL) return Time;

	-- Returns 0 if no new data available.
	function Read_Interrupt_Signal(Ctx: in out LL) return Time;
	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Wheel_Selection(Ctx: in out LL) return Wheel_Selection;
	-- return "percentage" scale value
	function Read_Light_Sensor(Ctx: in out LL) return Light_Value;

	procedure Set_Buzzer_Enabled(Ctx: in out LL; Enabled: in Boolean);

private

	procedure Handle_DCF_Interrupt(Pin: RP.GPIO.GPIO_Pin;
					Trigger: RP.GPIO.Interrupt_Triggers);

	type LL is tagged limited record
		ADC1_Light: RP.ADC.ADC_Channel;
		ADC2_Wheel: RP.ADC.ADC_Channel;
	end record;

	-- Digital Inputs
	Not_DCF:         RP.GPIO.GPIO_Point renames Pico.GP7;
	Not_Ta_L:        RP.GPIO.GPIO_Point renames Pico.GP8;
	Not_Ta_R:        RP.GPIO.GPIO_Point renames Pico.GP9;

	-- Analog Inputs
	Light:           RP.GPIO.GPIO_Point renames Pico.GP27;
	Wheel:           RP.GPIO.GPIO_Point renames Pico.GP28;

	-- Outputs
	Control_Or_Data: RP.GPIO.GPIO_Point renames Pico.GP6;
	Buzzer:          RP.GPIO.GPIO_Point renames Pico.GP10;

	-- SPI
	SCK:             RP.GPIO.GPIO_Point renames Pico.GP2;
	TX:              RP.GPIO.GPIO_Point renames Pico.GP3;
	RX:              RP.GPIO.GPIO_Point renames Pico.GP4;
	Not_CS:          RP.GPIO.GPIO_Point renames Pico.GP5;
	SPI_Port:        RP.SPI.SPI_Port    renames RP.Device.SPI_0;

	-- Static Variables for Interrupt Counter.
	-- Cannot be part of the record for lifecycle reasons
	-- ISR procedure may run even if context already destroyed!
	-- Marker value "0" is used to indicate "unset".
	-- After processing, users should reset the Interrupt_Out_Ticks to 0.
	Interrupt_Fault_Max: constant Natural := 1000;
	Interrupt_Start_Ticks: Time    := 0;
	Interrupt_Out_Ticks:   Time    := 0;
	Interrupt_Fault:       Natural := 0;

end DCF77_Low_Level;
