with Interfaces;

package DCF77_Types is
	
	-- technical types
	subtype U8  is Interfaces.Unsigned_8;
	subtype U16 is Interfaces.Unsigned_16;
	subtype U32 is Interfaces.Unsigned_32;

	-- logical types
	type Reading is (Bit_0, Bit_1, No_Signal, No_Update);

end DCF77_Types;
