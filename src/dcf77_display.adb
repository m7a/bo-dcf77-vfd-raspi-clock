with Interfaces;
use  Interfaces;

with DCF77_Display.Font;

package body DCF77_Display is

	procedure Init(Ctx: in out Disp; LL: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL                 := LL;
		Ctx.Vscreen            := 0;
		Ctx.Current_Brightness := Display_Brightness_Perc_100;
		-- Add 10ms startup delay before starting SPI comms to give the
		-- display a chance to initialize itself before the first
		-- command.
		Ctx.LL.Delay_Micros(10_000);
		Ctx.Send_Seq((
			-- mode / initialization
			(GP9002_Display,              Control),
			(GP9002_Display_Monochrome,   Data),
			-- brightness
			(GP9002_Bright,               Control),
			(Display_Brightness_Perc_100, Data),
			-- 2nd screen starts at 16#400#
			(GP9002_Loweraddr2,           Control),
			(16#00#,                      Data),
			(GP9002_Higheraddr2,          Control),
			(16#04#,                      Data),
			-- clear screen and show vscreen 0
			(GP9002_Clearscreen,          Control),
			(GP9002_Display1on,           Control)
		));
	end Init;

	procedure Send_Seq(Ctx: in out Disp; Seq: in Sequence) is
	begin
		for I of Seq loop
			Ctx.Send_U8(I);
		end loop;
	end Send_Seq;

	procedure Send_U8(Ctx: in out Disp; Seq: in Sequence_Member) is
		Val: constant Bytes(1 .. 1) := (1 => Reverse_Bits_8(Seq.Value));
	begin
		Ctx.LL.SPI_Display_Transfer_Reversed(Val, Seq.Mode);
		Ctx.LL.Delay_Micros(if Seq.Mode = Control and
				Seq.Value = GP9002_Clearscreen then 270 else 1);
	end Send_U8;

	function Reverse_Bits_8(V: in U8) return U8 is (
		Shift_Left(V and 16#01#, 7) or Shift_Right(V and 16#80#, 7) or
		Shift_Left(V and 16#02#, 5) or Shift_Right(V and 16#40#, 5) or
		Shift_Left(V and 16#04#, 3) or Shift_Right(V and 16#20#, 3) or
		Shift_Left(V and 16#08#, 1) or Shift_Right(V and 16#10#, 1));

	procedure Update(Ctx: in out Disp; It: in Items;
					New_Brightness: in Brightness
					:= Display_Brightness_Perc_100) is
		--use type U8;
		Old_Vscreen: constant U8 := Ctx.Vscreen;
	begin
		Ctx.Vscreen := 1 - Ctx.Vscreen;

		-- write to new screen
		for I of It loop
			Ctx.Add(I);
		end loop;

		-- update brightness if necessary
		if New_Brightness /= Ctx.Current_Brightness then
			Ctx.Send_Seq((
				(GP9002_Bright, Control),
				(New_Brightness, Data)
			));
			Ctx.Current_Brightness := New_Brightness;
		end if;

		-- display new screen
		case Ctx.Vscreen is
		when 0      => Ctx.Send_U8((GP9002_Display1on, Control));
		when 1      => Ctx.Send_U8((GP9002_Display2on, Control));
		when others => null; -- fault
		end case;

		-- clear old screen
		Ctx.Set_Address(Shift_Left(U16(Old_Vscreen), 10));
		Ctx.LL.SPI_Display_Transfer_Reversed(Blank_Data, Data);
	end Update;

	procedure Add(Ctx: in out Disp; Item: in Display_Item) is
		-- constants relevant for special case which is fixed to small
		-- font size for now
		Is_Special:   constant Boolean := Item.ULB or Item.ULT or
							Item.ULL or Item.ULR;

		-- for now we blindly assume y % 8 = 0!
		Addr:         U16 := U16(Item.X) * 8 + U16(Item.Y) / 8 +
					Shift_Left(U16(Ctx.Vscreen), 10);

		-- For simplicity draw special stuff always as small
		procedure Add_Special(C: in Character;
						Line_L, Line_R: in Boolean) is
			Line: U16;
			L_Array: Bytes(1 .. 2);
			for L_Array'Address use Line'Address;
		begin
			for RX in 1 .. Letter_Width loop
				Line := (if (RX = 1 and then Line_L) or else
					    (RX = Letter_Width and then Line_R)
					then (16#fffc#)
					else Font.Small_Data(C)(RX));
				if Item.ULB then
					Line := Line or 16#0004#;
				end if;
				if Item.ULT then
					Line := Line or 16#8000#;
				end if;
				Line := Reverse_Bits_16(Line);
				Ctx.Set_Address(Addr);
				Ctx.LL.SPI_Display_Transfer_Reversed(L_Array,
									Data);
				Ctx.LL.Delay_Micros(1);
				Addr := Addr + 8;
			end loop;
		end Add_Special;

		procedure Add_Small(C: in Character) is
			Line: U16;
			L_Array: Bytes(1 .. 2);
			for L_Array'Address use Line'Address;
		begin
			for RX in 1 .. Letter_Width loop
				Line := Reverse_Bits_16(Font.Small_Data(C)(RX));
				Ctx.Set_Address(Addr);
				Ctx.LL.SPI_Display_Transfer_Reversed(L_Array,
									Data);
				Ctx.LL.Delay_Micros(1);
				Addr := Addr + 8;
			end loop;
		end Add_Small;

		procedure Add_Large(C: in Character) is
			Value: constant Bytes :=
				(if C = ':' then Large_Font_Data(C)(1 .. 16)
				else Large_Font_Data(C));
		begin
			Ctx.Set_Address(Addr);
			Ctx.LL.SPI_Display_Transfer_Reversed(Value, Data);
			Ctx.LL.Delay_Micros(1);
			Addr := Addr + Value'Length + 8;
		end Add_Large;
	begin
		if Is_Special then
			for I in 1 .. SB.Length(Item.Msg) loop
				Add_Special(SB.Element(Item.Msg, I),
						I = 1 and then Item.ULL,
						I = SB.Length(Item.Msg)
							and then Item.ULR);
			end loop;
		else
			case Item.F is
			when Small =>
				for I in 1 .. SB.Length(Item.Msg) loop
					Add_Small(SB.Element(Item.Msg, I));
				end loop;
			when Large =>
				for I in 1 .. SB.Length(Item.Msg) loop
					Add_Large(SB.Element(Item.MSG, I));
				end loop;
			end case;
		end if;
	end Add;

	function Reverse_Bits_16(V: in U16) return U16 is (
		U16(Reverse_Bits_8(U8(Shift_Right(V and 16#ff00#, 8)))) or
		Shift_Left(U16(Reverse_Bits_8(U8(V and 16#00ff#))), 8));

	procedure Set_Address(Ctx: in out Disp; Addr: in U16) is
	begin
		Ctx.Send_Seq((
			(GP9002_Addrl,                         Control),
			(U8(Addr and 16#ff#),                  Data),
			(GP9002_Addrh,                         Control),
			(U8(Shift_Right(Addr and 16#f00#, 8)), Data),
			(GP9002_Datawrite,                     Control)
		));
	end Set_Address;

end DCF77_Display;
