with Ada.Strings.Bounded;

with DCF77_Types;
use  DCF77_Types;
with DCF77_Low_Level;
use  DCF77_Low_Level;

package DCF77_Display is

	package SB is new Ada.Strings.Bounded.Generic_Bounded_Length(Max => 17);

	subtype Brightness is U8;
	subtype Pos_X is U8 range 0 .. 127;
	subtype Pos_Y is U8 range 0 .. 63;

	type Font_Size is (Large, Small);

	type Display_Item is record
		X:   Pos_X;
		Y:   Pos_Y;
		Msg: SB.Bounded_String;
		F:   Font_Size := Small;
		-- underline bottom/top/left/right
		ULB: Boolean := False;
		ULT: Boolean := False;
		ULL: Boolean := False;
		ULR: Boolean := False;
	end record;

	type Items is array (Natural range <>) of Display_Item;

	type Disp is tagged limited private;

	Display_Brightness_Perc_100: constant Brightness := 16#00#;
	Display_Brightness_Perc_090: constant Brightness := 16#06#;
	Display_Brightness_Perc_080: constant Brightness := 16#0c#;
	Display_Brightness_Perc_070: constant Brightness := 16#12#;
	Display_Brightness_Perc_060: constant Brightness := 16#18#;
	Display_Brightness_Perc_050: constant Brightness := 16#1e#;
	Display_Brightness_Perc_040: constant Brightness := 16#24#;
	Display_Brightness_Perc_030: constant Brightness := 16#2a#;
	Display_Brightness_Perc_000: constant Brightness := 16#ff#;

	procedure Init(Ctx: in out Disp; LL: in DCF77_Low_Level.LLP);
	procedure Update(Ctx: in out Disp; It: in Items;
		New_Brightness: in Brightness := Display_Brightness_Perc_100);
	function Get_Letter_Width(F: in Font_Size) return Pos_X;

private

	-- Translated from from https://github.com/adafruit/
	-- Adafruit-Graphic-VFD-Display-Library/blob/master/Adafruit_GP9002.h
	GP9002_Displaysoff        : constant U8 := 16#00#;
	GP9002_Display1on         : constant U8 := 16#01#;
	GP9002_Display2on         : constant U8 := 16#02#;
	GP9002_Addrincr           : constant U8 := 16#04#;
	GP9002_Addrheld           : constant U8 := 16#05#;
	GP9002_Clearscreen        : constant U8 := 16#06#;
	GP9002_Controlpower       : constant U8 := 16#07#;
	GP9002_Datawrite          : constant U8 := 16#08#;
	GP9002_Dataread           : constant U8 := 16#09#;
	GP9002_Loweraddr1         : constant U8 := 16#0A#;
	GP9002_Higheraddr1        : constant U8 := 16#0B#;
	GP9002_Loweraddr2         : constant U8 := 16#0C#;
	GP9002_Higheraddr2        : constant U8 := 16#0D#;
	GP9002_Addrl              : constant U8 := 16#0E#;
	GP9002_Addrh              : constant U8 := 16#0F#;
	GP9002_Or                 : constant U8 := 16#10#;
	GP9002_Xor                : constant U8 := 16#11#;
	GP9002_And                : constant U8 := 16#12#;
	-- 3-5-10. Luminance Adjustment
	GP9002_Bright             : constant U8 := 16#13#;
	GP9002_Display            : constant U8 := 16#14#;
	GP9002_Display_Monochrome : constant U8 := 16#10#;
	GP9002_Display_Grayscale  : constant U8 := 16#14#;
	GP9002_Intmode            : constant U8 := 16#15#;
	GP9002_Drawchar           : constant U8 := 16#20#;
	GP9002_Charram            : constant U8 := 16#21#;
	GP9002_Charsize           : constant U8 := 16#22#;
	GP9002_Charbright         : constant U8 := 16#24#;

	type Disp is tagged limited record
		LL:                 DCF77_Low_Level.LLP;
		Vscreen:            U8;
		Current_Brightness: Brightness;
	end record;

	type Sequence_Member is record
		Value: U8;
		Mode:  SPI_Display_Mode;
	end record;

	type Sequence is array (Natural range <>) of Sequence_Member;

	procedure Send_Seq(Ctx: in out Disp; Seq: in Sequence);
	procedure Send_U8(Ctx: in out Disp; Seq: in Sequence_Member);
	procedure Add(Ctx: in out Disp; Item: in Display_Item);
	procedure Set_Address(Ctx: in out Disp; Addr: in U16);
	procedure Write_Zero(Ctx: in out Disp; Addr: in U16; N: in U16);

end DCF77_Display;
