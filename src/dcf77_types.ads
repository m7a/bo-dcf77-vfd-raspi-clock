with Interfaces;

package DCF77_Types is
	
	-- technical types
	subtype U8  is Interfaces.Unsigned_8;
	subtype U16 is Interfaces.Unsigned_16;
	subtype U32 is Interfaces.Unsigned_32;

	-- logical types
	type Reading is (Bit_0, Bit_1, No_Signal, No_Update);
	type Bits    is array (Natural range <>) of Reading;

	-- http://computer-programming-forum.com/44-ada/0771fd8bfc850971.htm
	-- Perform static asserts as follows:
	-- A: constant Static_Assert := Static_Assert'(EXPRESSION);
	subtype Static_Assert is Boolean range True..True;

end DCF77_Types;
