with RP.Clock;
with HAL.SPI;

package body DCF77_Low_Level is

	procedure Init(Ctx: in out LL) is
	begin
		-- Clocks and Timers
		RP.Clock.Initialize(Pico.XOSC_Frequency);
		RP.Clock.Enable(RP.Clock.ADC); -- ADC
		RP.Clock.Enable(RP.Clock.PERI); -- SPI
		RP.Device.Timer.Enable;

		-- Digital Inputs
		Not_DCF.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_DCF.Set_Interrupt_Handler(Handle_DCF_Interrupt'Access);
		Not_Ta_L.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);
		Not_Ta_R.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up);

		-- Analog Inputs
		Light.Configure(RP.GPIO.Input, RP.GPIO.Floating);
		Light.Configure(RP.GPIO.Analog);
		Ctx.ADC1_Light := RP.ADC.To_ADC_Channel(Light);
		RP.ADC.Configure(Ctx.ADC1_Light);

		Wheel.Configure(RP.GPIO.Input, RP.GPIO.Floating);
		Wheel.Configure(RP.GPIO.Analog);
		Ctx.ADC2_Wheel := RP.ADC.To_ADC_Channel(Wheel);
		RP.ADC.Configure(Ctx.ADC2_Wheel);

		-- Outputs
		Control_Or_Data.Configure(RP.GPIO.Output);
		Buzzer.Configure(RP.GPIO.Output);

		-- SPI (https://pico-doc.synack.me/#spi)
		SCK.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		TX.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		RX.Configure(RP.GPIO.Input, RP.GPIO.Pull_Up, RP.GPIO.SPI);
		Not_CS.Configure(RP.GPIO.Output, RP.GPIO.Floating, RP.GPIO.SPI);
		-- if it matters, Arduino has DORD - data order lsb first?
		SPI_Port.Configure(Config => (
			Role      => RP.SPI.Master,
			Baud      => 1_000_000,            -- 1MHz, up to 50MHz!
			Data_Size => HAL.SPI.Data_Size_8b, -- not on Arduino
			Polarity  => RP.SPI.Active_Low,    -- clock idle at 1
			Phase     => RP.SPI.Rising_Edge,   -- sample on leading
			Blocking  => True
		));
	end Init;
	
	procedure Handle_DCF_Interrupt(Pin: RP.GPIO.GPIO_Pin;
					Trigger: RP.GPIO.Interrupt_Triggers) is
		-- TODO MOVE TO SUPPORT MODULE / Maymbe dedicated counter 0..ff or 0.999 or such?
		procedure Inc_Saturated(Ctr: in out Natural; Lim: in Natural) is
		begin
			if Ctr < Lim then
				Ctr := Ctr + 1;
			end if;
		end Inc_Saturated;
	begin
		case Trigger is
		when RP.GPIO.Rising_Edge =>
			-- Input voltage 0/1 transition means DCF77 1/0
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
		when RP.GPIO.Falling_Edge =>
			-- Input voltage 1/0 transition means DCF77 0/1
			-- transition. Begin of a "high" signal
			if Interrupt_Start_Ticks = 0 then
				Interrupt_Start_Ticks := Time(RP.Timer.Clock);
			else
				Inc_Saturated(Interrupt_Fault,
							Interrupt_Fault_Max);
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

	function Read_Interrupt_Signal(Ctx: in out LL) return Time is
		Ret: constant Time := Interrupt_Out_Ticks;
	begin
		if Ret /= 0 then
			-- mark that the data was processed successfully
			Interrupt_Out_Ticks := 0;
		end if;
		return Ret;
	end Read_Interrupt_Signal;

	function Read_Left_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_L.Get);
	function Read_Right_Button_Is_Down(Ctx: in out LL) return Boolean is
							(not Not_Ta_R.Get);

	function Read_Wheel_Selection(Ctx: in out LL) return Wheel_Selection is (Wheel_Default); -- TODO
	function Read_Light_Sensor(Ctx: in out LL) return Light_Value is (0); -- TODO

end DCF77_Low_Level;
