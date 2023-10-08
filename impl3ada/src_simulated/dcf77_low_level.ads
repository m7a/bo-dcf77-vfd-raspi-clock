with DCF77_Types;
use  DCF77_Types;

-- TODO PROVIDE AN IMPLEMENTATION FOR TESTING

package DCF77_Low_Level is

	type SPI_Display_Mode is (Control, Data);

	type Time        is new RP.Timer.Time;
	type Light_Value is new Integer range 0 .. 100;

	type LL is tagged limited private;

	procedure Init(Ctx: in out LL);

	function Get_Time_Micros(Ctx: in out LL) return Time;
	procedure Delay_Micros(Ctx: in out LL; DT: in Time);

	-- Returns False if no new data available. Clears data
	function Read_Interrupt_Signal(Ctx: in out LL; Signal_Length: out Time;
					Signal_Begin: out Time) return Boolean;

	-- Returns True when Button is held down
	function Read_Green_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean;
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean;

	-- Returns "percentage" scale value
	function Read_Light_Sensor(Ctx: in out LL) return Light_Value;

	procedure Set_Buzzer_Enabled(Ctx: in out LL; Enabled: in Boolean);
	procedure Set_Alarm_LED_Enabled(Ctx: in out LL; Enabled: in Boolean);

	-- Display management for common word sizes
	-- These write with data order msb first (most significant bit first)
	-- The actual display expects lsb first -> must convert before using
	-- these transfer functions.
	procedure SPI_Display_Transfer(Ctx: in out LL; Send_Value: in U8;
						Mode: in SPI_Display_Mode);
	procedure SPI_Display_Transfer(Ctx: in out LL; Send_Value: in U16;
						Mode: in SPI_Display_Mode);
	procedure SPI_Display_Transfer(Ctx: in out LL; Send_Value: in U32;
						Mode: in SPI_Display_Mode);

	function Get_Fault(Ctx: in out LL) return Natural;

	procedure Log(Ctx: in out LL; Msg: in String);

	procedure Debug_Dump_Interrupt_Info(Ctx: in out LL);

	procedure Simul_Set_Input_Status(
			Green_Button, Left_Button, Right_Button: Boolean;
			Light_Sensor: Light_Value);
	procedure Simul_Query_Output_Status(Buzzer, Alarm_LED: out Boolean);

private

	-- inputs
	Green_Button_Down:   Boolean     := False;
	Left_Button_Down:    Boolean     := False;
	Right_Button_Down:   Boolean     := False;
	Light_Sensor_Status: Light_Value := 0;

	-- outputs
	Buzzer_Enabled:      Boolean     := False;
	Alarm_LED_Enabled:   Boolean     := False;

	type LL is tagged limited record with null;

end DCF77_Low_Level;
