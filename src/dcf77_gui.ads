with DCF77_Display;
use  DCF77_Display;
with DCF77_Types;
with DCF77_Bitlayer;
with DCF77_SM_Layer_Shared;
with DCF77_Secondlayer;
with DCF77_Minutelayer;
with DCF77_Timelayer;
with DCF77_Low_Level;
with DCF77_Ambient_Light_Sensor;
with DCF77_Alarm;
with DCF77_TM_Layer_Shared;

package DCF77_GUI is

	procedure Mainloop;

private

	-- Must keep this as a singleton for access reasons
	LLI: aliased DCF77_Low_Level.LL;

	type Program_State is tagged limited record
		-- Low Level
		LL:                     DCF77_Low_Level.LLP;
		Disp:                   aliased DCF77_Display.Disp;

		-- Timekeeping
		Bitlayer:               aliased DCF77_Bitlayer.Bitlayer;
		Secondlayer:            aliased DCF77_Secondlayer.Secondlayer;
		Minutelayer:            aliased DCF77_Minutelayer.Minutelayer;
		Timelayer:              aliased DCF77_Timelayer.Timelayer;
		-- Support Layers
		ALS:                    aliased DCF77_Ambient_Light_Sensor.ALS;
		Alarm:                  aliased DCF77_Alarm.Alarm;

		-- State Transfer between Layers
		Light_Sensor_Reading:   DCF77_Low_Level.Light_Value;
		Brightness_Setting:     DCF77_Display.Brightness;
		Bitlayer_Reading:       DCF77_Types.Reading;
		Secondlayer_Telegram_1: DCF77_SM_Layer_Shared.Telegram;
		Secondlayer_Telegram_2: DCF77_SM_Layer_Shared.Telegram;
		Datetime:               DCF77_TM_Layer_Shared.TM;
	end record;

	procedure Init(S: in out Program_State);
	procedure Loop_Pre(S: in out Program_State);

	Max_Num_Items: constant Natural := 20;

	type State is (
		Select_Display,
			Select_Alarm, Select_AL_H, Select_AL_I,
		Select_Datetime,
			Select_DT_H,  Select_DT_I, Select_DT_S,
			Select_DT_YH, Select_DT_Y, Select_DT_M, Select_DT_D,
		Select_Info,
			Select_I_QOS, Select_I_Bits, Select_I_Bits_Oszi,
			Select_I_Last_1, Select_I_Last_2,
		Select_Version,
			Select_Ver_2, Select_Ver_3
	);

	type Blink is mod 4;

	Blink_Lim: constant Blink := 1;

	type GUI is tagged limited record
		S: aliased Program_State;

		A:                       State;
		Numeric_Editing_Enabled: Boolean;
		Blink_Value:             Blink;
		Screen:                  Items(1 .. Max_Num_Items);
		Screen_Idx:              Natural;

		-- Button Values last time
		Last_Green:              Boolean;
		Last_Left:               Boolean;
		Last_Right:              Boolean;

		-- reserved for future use. remove once something meaningful
		-- is established behind this option...
		RFU_Option:              Boolean;
	end record;

	-- 1..N index to underline, <= 0: special meaning, see constants
	type Underline_Info is new Integer range -3 .. 6;

	Underline_BTLR: constant Underline_Info := -3;
	Underline_BLR:  constant Underline_Info := -2;
	Underline_TLR:  constant Underline_Info := -1;
	Underline_None: constant Underline_Info :=  0;

	-- Edit := Edit/Save depending on state of Numeric_Editing_Enabled
	type Menu_Green is (Menu_Next, Menu_Edit, Menu_Toggle, Menu_Home);

	procedure Init(G: in out GUI);
	procedure Process(G: in out GUI);

-- private GUI-internal functions --

	procedure Process_Editing_Inputs(G: in out GUI;
					Finish, Left, Right: in Boolean);
	procedure Process_Navigation_Inputs(G: in out GUI;
					Next_Edit, Left, Right: in Boolean);
	procedure Update_Screen(G: in out GUI);
	procedure Add_Main_Display(G: in out GUI);
	procedure Add_Time_Small(G: in out GUI; XI: in Pos_X; YI: in Pos_Y;
						Underline: in Underline_Info);
	procedure Add_Date(G: in out GUI; Underline: in Underline_Info);
	procedure Add_QOS(G: in out GUI; XI: in Pos_X; YI: in Pos_Y);
	procedure Add_AL(G: in out GUI; YI: in Pos_Y;
						Underline: in Underline_Info);
	procedure Add_Menu(G: in out GUI; Lbl: in Menu_Green);
	procedure Add_Info_Ctr(G: in out GUI);
	procedure Add_Info(G: in out GUI; Title, L1, L2: in String);
	procedure Add_Info_Bits(G: in out GUI);
	procedure Add_Info_Oszi(G: in out GUI);
	procedure Add_Last_1(G: in out GUI);
	procedure Add_Last_2(G: in out GUI);

end DCF77_GUI;
