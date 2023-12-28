with DCF77_Display;
use  DCF77_Display;
with DCF77_Ticker;
with DCF77_Types;
with DCF77_Bitlayer;
with DCF77_ST_Layer_Shared;
with DCF77_Secondlayer;
with DCF77_Timelayer;
with DCF77_Low_Level;
with DCF77_Alarm;

package DCF77_GUI is

	procedure Mainloop;

private

	-- Must keep this as a singleton for access reasons
	LLI: aliased DCF77_Low_Level.LL;

	type Program_State is tagged limited record
		-- Low Level
		LL:          DCF77_Low_Level.LLP;
		Disp:        aliased DCF77_Display.Disp;

		-- Timekeeping
		Ticker:      aliased DCF77_Ticker.Ticker;
		Bitlayer:    aliased DCF77_Bitlayer.Bitlayer;
		Secondlayer: aliased DCF77_Secondlayer.Secondlayer;
		Timelayer:   aliased DCF77_Timelayer.Timelayer;

		Alarm:       aliased DCF77_Alarm.Alarm;

		-- State Transfer between Layers
		Bitlayer_Reading:       DCF77_Types.Reading;
		Secondlayer_Telegram_1: DCF77_ST_Layer_Shared.Telegram;
		Secondlayer_Telegram_2: DCF77_ST_Layer_Shared.Telegram;
		Datetime:               DCF77_Timelayer.TM;
	end record;

	procedure Init(S: in out Program_State);
	procedure Loop_Pre(S: in out Program_State);
	procedure Loop_Post(S: in out Program_State);

	-- Well Known Indices for the Screens
	-- :Date: 2023-12-28
	-- :AL:   AL09:20
	-- :QOS:  +1
	-- :Time: 00:55:55
	-- :M_:   Menu Entries in Navigation Line (last line)
	WIDX_Date:    constant Natural := 1;
	WIDX_AL:      constant Natural := 2;
	WIDX_QOS:     constant Natural := 3;
	WIDX_Time:    constant Natural := 4;
	WIDX_M_Next:  constant Natural := 5;
	WIDX_M_Minus: constant Natural := 6;
	WIDX_M_Plus:  constant Natural := 7;
	WIDX_M_AL:    constant Natural := 8;

	type State is (
		Select_Display,
			Select_Alarm, Select_AL_H, Select_AL_I,
		Select_Datetime,
			Select_DT_H,  Select_DT_I, Select_DT_S,
			Select_DT_YH, Select_DT_Y, Select_DT_M, Select_DT_D,
		Select_Options,
		Select_Info
	);

	type GUI is tagged limited record
		S: aliased Program_State;

		-- GUI Automaton State
		A:                       State;
		Numeric_Editing_Enabled: Boolean;
		Blink_Value:             Boolean;

		-- Screens
		Screen_Display: Items(WIDX_Date .. WIDX_Time);
		Screen_Edit:    Items(WIDX_Date .. WIDX_M_AL);
		Screen_Options: Items(WIDX_Time .. WIDX_M_AL); -- TODO +x
		Screen_Info:    Items(WIDX_Time .. WIDX_M_AL); -- TODO SZ TBD
		-- TODO ... GUI STUFF GOES HERE
	end record;

	procedure Init(G: in out GUI);
	procedure Process(G: in out GUI);

-- GUI private stuff follows --

	procedure Format_Datetime(T: in DCF77_Timelayer.TM;
					Date, Time: out SB.Bounded_String);
	procedure Format_Time(T: in DCF77_Timelayer.TM;
					Time: out SB.Bounded_String);
	procedure Update_Display(G: in out GUI);
	function Describe_QOS(Q: in DCF77_Timelayer.QOS)
						return SB.Bounded_String;
	function Describe_AL(G: in GUI) return SB.Bounded_String;

end DCF77_GUI;
