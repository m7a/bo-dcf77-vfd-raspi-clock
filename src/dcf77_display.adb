with Interfaces;
use  Interfaces;

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
		use type U8;
	begin
		Ctx.LL.SPI_Display_Transfer(Seq.Value, Seq.Mode);
		Ctx.LL.Delay_Micros(if Seq.Mode = Control and
				Seq.Value = GP9002_Clearscreen then 270 else 1);
	end Send_U8;

	procedure Update(Ctx: in out Disp; It: in Items;
					New_Brightness: in Brightness
					:= Display_Brightness_Perc_100) is
		use type U8;
		Old_Vscreen: constant U8 := Ctx.Vscreen;
	begin
		Ctx.Vscreen := 1 - Ctx.Vscreen;

		-- write to new screen
		for I of It loop
			Ctx.Add(I);
		end loop;

		-- update brightness if necessary
		if New_Brightness /= Ctx.Current_Brightness then
			Ctx.Send_U8((GP9002_Bright, Control));
			Ctx.Send_U8((New_Brightness, Data));
			Ctx.Current_Brightness := New_Brightness;
		end if;

		-- display new screen
		case Ctx.Vscreen is
		when 0      => Ctx.Send_U8((GP9002_Display1on, Control));
		when 1      => Ctx.Send_U8((GP9002_Display2on, Control));
		when others => null; -- fault
		end case;

		-- clear old screen
		Ctx.Write_Zero(Shift_Left(U16(Old_Vscreen), 10), 16#400#);
	end Update;

	procedure Add(Ctx: in out Disp; Item: in Display_Item) is
		use type U32;
		-- for now we blindly assume y % 8 = 0!
		Addr:         U16 := U16(Item.X) * 8 + U16(Item.Y) / 8 +
					Shift_Left(U16(Ctx.Vscreen), 10);
		Letter_Width: Integer;
		C:            Character;
	begin
		case Item.F is
		when Small => Letter_Width :=  8;
		when Large => Letter_Width := 16;
		end case;

		for I in 1 .. SB.Length(Item.Msg) loop
			C := SB.Element(Item.Msg, I);
			for RX in 1 .. Letter_Width loop
				Ctx.Set_Address(Addr);
				case Item.F is
				when Small => Ctx.LL.SPI_Display_Transfer(
					Font_Small_Data(C)(RX), Data);
				when Large => Ctx.LL.SPI_Display_Transfer(
					Font_Large_Data(C)(RX), Data);
				end case;
				Ctx.LL.Delay_Micros(1);
				Addr := Addr + 8;
			end loop;
		end loop; 
	end Add;

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

	procedure Write_Zero(Ctx: in out Disp; Addr: in U16; N: in U16) is
	begin
		Ctx.Set_Address(Addr);
		for I in 1 .. N loop
			Ctx.Send_U8((16#00#, Data));
		end loop;
	end Write_Zero;

end DCF77_Display;
