with Interfaces;
use  Interfaces;

package body DCF77_Display is

	procedure Init(Ctx: in out Disp; LL: access DCF77_Low_Level.LL) is
	begin
		Ctx.LL                 := LL;
		Ctx.Vscreen            := 0;
		Ctx.Current_Brightness := Display_Brightness_Perc_100;
		-- Add 10ms startup delay before starting SPI comms to give the
		-- display a chance to initialize itself before the first
		-- command. TODO TEST IF WE CAN REDUCE THIS TO 1ms?
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
		-- TODO DEBUG ONLY -- TODO TEST IF WE CAN DO WITHOUT THIS
		--                    (clearscreen should suffice?)
		-- Zero out the entire memory to clear both screens
		--Ctx.Write_Zero(0, 16#800#);
		Ctx.Set_Address(0);
		for I in 1 .. 16#400# loop
			Ctx.Send_U8((16#a5#, Data));
		end loop;
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
		Ctx.LL.SPI_Display_Transfer(Reverse_Bits(Seq.Value), Seq.Mode);
		Ctx.LL.Delay_Micros(if Seq.Mode = Control and
				Seq.Value = GP9002_Clearscreen then 270 else 1);
	end Send_U8;

	-- The low level functions (chip) only support msb first, but the
	-- display module requires lsb first. These conversion function should
	-- do the job clearly and efficiently. If they are needed in another
	-- place, may as well move them to dcf77_types or dcf77_functions...
	function Reverse_Bits(V: in U8) return U8 is (
		Shift_Left (V and 16#01#, 7) or Shift_Right(V and 16#80#, 7) or
		Shift_Left (V and 16#02#, 5) or Shift_Right(V and 16#40#, 5) or
		Shift_Left (V and 16#04#, 3) or Shift_Right(V and 16#20#, 3) or
		Shift_Left (V and 16#08#, 1) or Shift_Right(V and 16#10#, 1));

	function Reverse_Bits(V: in U16) return U16 is (
		U16(Reverse_Bits(U8(Shift_Right(V and 16#ff00#, 8)))) or
		Shift_Left(U16(Reverse_Bits(U8(V and 16#00ff#))), 8));

	function Reverse_Bits(V: in U32) return U32 is (
		U32(Reverse_Bits(U16(Shift_Right(V and 16#ffff0000#, 16)))) or
		Shift_Left(U32(Reverse_Bits(U16(V and 16#0000ffff#))), 16));

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
					Reverse_Bits(Font_Small_Data(C)(RX)),
					Data);
				when Large => Ctx.LL.SPI_Display_Transfer(
					Reverse_Bits(Font_Large_Data(C)(RX)),
					Data);
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
