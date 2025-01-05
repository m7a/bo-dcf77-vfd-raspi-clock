with DCF77_Functions;
with DCF77_Low_Level;
use  DCF77_Low_Level;

with Interfaces;
use  Interfaces; -- Operators for U32?

package body DCF77_Bitlayer is

	procedure Init(Ctx: in out Bitlayer; LL: in DCF77_Low_Level.LLP) is
	begin
		Ctx.LL             := LL;
		Ctx.Start_Of_Slice := LL.Get_Time_Micros;
		Ctx.Current_State  := S_Z0;
		Ctx.Buf_Ctr        := 0;
		Ctx.Last_Input     := I_ANY_OTHER;
		Ctx.Last_Raw       := 0;
		Ctx.Last_Filtered  := 0;
		Ctx.Delay_Us       := Delay_Us_Target;
		Ctx.Unidentified   := 0;
		Ctx.Overflown      := 0;
	end Init;

	function Update_Tick(Ctx: in out Bitlayer) return Reading is
	begin
		Ctx.Align_To_Slice;
		return Ctx.Update_Main_State;
	end Update_Tick;

	-- Slices are the tick interval at which the display is updated and the
	-- interrupt readings are queried. Here, it is aligned to be every 100ms
	-- automatically adjusting to the computation time that the preceding
	-- code took.
	procedure Align_To_Slice(Ctx: in out Bitlayer) is
		Time_Now: Time := Ctx.LL.Get_Time_Micros;
		End_Of_Slice: constant Time := Ctx.Start_Of_Slice +
								Delay_Us_Target;
	begin
		if Time_Now > End_Of_Slice then
			DCF77_Functions.Inc_Saturated(Ctx.Overflown, 99);
		else
			Ctx.Delay_Us := End_Of_Slice - Time_Now;
			if Time_Now < End_Of_Slice then
				Ctx.LL.Delay_Until(End_Of_Slice);
			end if;
			Time_Now := Ctx.LL.Get_Time_Micros;
		end if;
		Ctx.Start_Of_Slice := Time_Now;
	end Align_To_Slice;

	function Update_Main_State(Ctx: in out Bitlayer) return Reading is
		Ctr:    Natural;
		TR:     Transition;
		Result: Reading;

		procedure Invalid_Unidentified is
		begin
			Ctx.LL.Log("Unidentified: " &
						Natural'Image(Ctx.Buf_Ctr));
			DCF77_Functions.Inc_Saturated(Ctx.Unidentified, 9999);

			Result            := TR.G;
			Ctx.Buf_Ctr       := 0;
			Ctx.Current_State := TR.T(R_Default);
		end Invalid_Unidentified;
	begin
		-- these values are stored on CTR level only for the purpose
		-- of displaying them in the GUI for debug purposes
		Ctx.Last_Raw      := Ctx.LL.Read_Interrupt_Signal_And_Clear;
		Ctx.Last_Filtered := Filter_And_Validate(Ctx.Last_Raw);
		Ctx.Last_Input    := Analyze_And_Count(Ctx.Last_Filtered, Ctr);

		TR := Transitions(Ctx.Current_State)(Ctx.Last_Input);

		case TR.A is
		when A_Ignore =>
			Result            := TR.G;
			Ctx.Buf_Ctr       := 0;
			Ctx.Current_State := TR.T(R_Default);
		when A_Append =>
			Ctx.Buf_Ctr := Ctx.Buf_Ctr + Ctr;
			if Ctx.Buf_Ctr < 35 then -- < 245ms
				-- could still be valid
				Result            := No_Update;
				Ctx.Current_State := TR.T(R_0);
			else
				Invalid_Unidentified;
			end if;
		when A_Decode =>
			Ctx.Buf_Ctr := Ctx.Buf_Ctr + Ctr;
			if Ctx.Buf_Ctr in 7 .. 20 then     -- 49  .. 140ms
				-- valid 0
				Result            := Bit_0;
				Ctx.Buf_Ctr       := 0;
				Ctx.Current_State := TR.T(R_0);
			elsif Ctx.Buf_Ctr in 21 .. 35 then -- 147 .. 245ms
				-- valid 1
				Result            := Bit_1;
				Ctx.Buf_Ctr       := 0;
				Ctx.Current_State := TR.T(R_1);
			else
				Invalid_Unidentified;
			end if;
		end case;
		return Result;
	end Update_Main_State;

	-- Filter and Validate State Chart
	--                                                  0/0
	--                                                  /\
	--                                      0/00        \v
	--                        --> [S1x0s] ----------> [SNx0]
	--                       /       ^  \             /   ^
	--                    0 /        |   \1         1/    | 0/00
	--   0                 /          \   \_______  /     |           0/1
	-- | /\               /            \________  \/   ->[S1x0b]      /\
	-- v \v   1/1        /                      \ /\  /        \1     \v
	-- Start -----> Data                _________/  \/          ---> error
	--                   \             /          \ /\         /0
	--                    \           /    ________/  -->[S1x1b]
	--                    1\         /    /         \     |
	--                      \        |  0/          0\    | 1/11
	--                       \       v  /             \   v
	--                        --> [S1x1s] ----------> [SNx1]
	--                                                 /^
	--                                                 \/
	--                                                 1/1
	--
	-- Filter the input and drop any single 0/1s returning a filtered value
	-- where at least two 0/1 come after each other making them eglegible to
	-- play a role for X1/X0/0/1 consideration.
	-- Returns 0 if invalid and filtered value if valid.
	function Filter_And_Validate(IRD: in U32) return U32 is
		function Is_0(M: in U32) return Boolean is ((IRD and M) = 0);
		function SHL(V: in U32; N: in Natural) return U32 renames
								Shift_Left;

		function Start(M: in U32)              return U32;
		function Data(M:  in U32; Acc: in U32) return U32;
		function S1x0s(M: in U32; Acc: in U32) return U32;
		function S1x1s(M: in U32; Acc: in U32) return U32;
		function SNx0(M:  in U32; Acc: in U32) return U32;
		function S1x0b(M: in U32; Acc: in U32) return U32;
		function SNx1(M:  in U32; Acc: in U32) return U32;
		function S1x1b(M: in U32; Acc: in U32) return U32;

		function Start(M: in U32) return U32 is begin
		if M = 0      then return 0; -- not accepting state
		elsif Is_0(M) then return Start(Next_M(M));
		else               return Data(Next_M(M), 1);
		end if; end Start;

		function Data(M: in U32; Acc: in U32) return U32 is begin
		if M = 0      then return 0; -- not an accepting state
		elsif Is_0(M) then return S1x0s(Next_M(M), Acc);
		else               return S1x1s(Next_M(M), Acc);
		end if; end Data;

		function S1x0s(M: in U32; Acc: in U32) return U32 is begin
		if M = 0 then      return Acc;
		elsif Is_0(M) then return SNx0(Next_M(M), SHL(Acc, 2));
		else               return S1x1b(Next_M(M), Acc);
		end if; end S1x0s;

		function S1x1s(M: in U32; Acc: in U32) return U32 is begin
		if M = 0      then return Acc;
		elsif Is_0(M) then return S1x0b(Next_M(M), Acc);
		else               return SNx1(Next_M(M), SHL(Acc, 2) or 3);
		end if; end S1x1s;

		function SNx0(M: in U32; Acc: in U32) return U32 is begin
		if M = 0      then return Acc;
		elsif Is_0(M) then return SNx0(Next_M(M), SHL(Acc, 1));
		else               return S1x1s(Next_M(M), Acc);
		end if; end SNx0;

		function S1x0b(M: in U32; Acc: in U32) return U32 is begin
		if M = 0      then return Acc;
		elsif Is_0(M) then return SNx0(Next_M(M), SHL(Acc, 2));
		else               return 0; -- error state
		end if; end S1x0b;

		function SNx1(M: in U32; Acc: in U32) return U32 is begin
		if M = 0      then return Acc;
		elsif Is_0(M) then return S1x1s(Next_M(M), Acc);
		else               return SNx1(Next_M(M), SHL(Acc, 1) or 1);
		end if; end SNx1;

		function S1x1b(M: in U32; Acc: in U32) return U32 is begin
		if M = 0      then return Acc;
		elsif Is_0(M) then return 0; -- error state
		else               return SNx1(Next_M(M), SHL(Acc, 2) or 3);
		end if; end S1x1b;
	begin
		return Start(16#8000_0000#);
	end Filter_And_Validate;

	function Next_M(Mask: in U32) return U32 is (Shift_Right(Mask, 1));

	-- Analyze and Count State Chart
	--
	--                           0              1/ctr++
	--                           /\               /\
	--                           \v    1/ctr++    \v
	--   0               0 ---> [S0] ----------> [SX1] ---- 0    0,1
	-- | /\               /                                \     /\
	-- v \v    1         /                                  \    \v
	-- Start -----> Data                                     --> any
	--                   \                                  /
	--             1/ctr++\            0                   /
	--                     ---> [S1] ----------> [SX0] ---- 1
	--                           /^               /^
	--                           \/               \/
	--                         1/ctr++            0
	--
	-- Check which Input case the given bits correspond to.
	-- If the result contains a series of ones (1, X1, X0) cases
	-- the number of consecutive ones is reflected in the Ctr out parameter.
	function Analyze_And_Count(FRD: in U32; Ctr: out Natural)
								return Input is
		function Is_0(M: in U32) return Boolean is ((FRD and M) = 0);

		function Start(M: in U32) return Input;
		function Data(M:  in U32) return Input;
		function S0(M:    in U32) return Input;
		function SX1(M:   in U32) return Input;
		function S1(M:    in U32) return Input;
		function SX0(M:   in U32) return Input;

		function Start(M: in U32) return Input is begin
		if M = 0      then   return I_ANY_OTHER; -- not accepting
		elsif Is_0(M) then   return Start(Next_M(M));
		else                 return Data(Next_M(M));
		end if; end Start;

		function Data(M: in U32) return Input is begin
		if M = 0      then   return I_ANY_OTHER; -- not accepting
		elsif Is_0(M) then   return S0(Next_M(M));
		else Ctr := Ctr + 1; return S1(Next_M(M));
		end if; end Data;

		function S0(M: in U32) return Input is begin
		if M = 0      then   return I_0;
		elsif Is_0(M) then   return S0(Next_M(M));
		else Ctr := Ctr + 1; return SX1(Next_M(M));
		end if; end S0;

		function SX1(M: in U32) return Input is begin
		if M = 0      then   return I_X1;
		elsif Is_0(M) then   return I_ANY_OTHER; -- any state
		else Ctr := Ctr + 1; return SX1(Next_M(M));
		end if; end SX1;

		function S1(M: in U32) return Input is begin
		if M = 0      then   return I_1;
		elsif Is_0(M) then   return SX0(Next_M(M));
		else Ctr := Ctr + 1; return S1(Next_M(M));
		end if; end S1;

		function SX0(M: in U32) return Input is begin
		if M = 0      then   return I_X0;
		elsif Is_0(M) then   return SX0(Next_M(M));
		else                 return I_ANY_OTHER; -- any state
		end if; end SX0;
	begin
		Ctr := 0;
		return Start(16#8000_0000#);
	end Analyze_And_Count;

	function Get_Unidentified(Ctx: in Bitlayer) return Natural is
							(Ctx.Unidentified);
	function Get_Overflown(Ctx: in Bitlayer) return Natural is
							(Ctx.Overflown);
	function Get_Delay(Ctx: in Bitlayer) return Time is (Ctx.Delay_Us);
	function Get_Counter(Ctx: in Bitlayer) return Natural is (Ctx.Buf_Ctr);

	function Get_Input(Ctx: in Bitlayer) return String is
	begin
		case Ctx.Last_Input is
		when I_1         => return "  1";
		when I_X1        => return " X1";
		when I_0         => return "  0";
		when I_X0        => return " X0";
		when I_ANY_OTHER => return "ANY";
		end case;
	end Get_Input;

	function Get_State(Ctx: in Bitlayer) return String is
		SR: String := State'Image(Ctx.Current_State);
	begin
		if SR'Length = 4 then
			SR(SR'First + 1) := ' ';
			return SR(SR'First + 1 .. SR'Last);
		elsif SR'Length = 5 then
			return SR(SR'First + 2 .. SR'Last);
		elsif SR'Length /= 3 then
			return "???";
		else
			return SR;
		end if;
	end Get_State;

	procedure Draw_Bits_State(Ctx: in Bitlayer; L1, L2: in out String) is
		C: constant Coordinate := Transition_Vis(Ctx.Current_State);
	begin
		L1(1  .. 16) := (others => ' ');
		L2(1  .. 14) := (others => ' ');
		L2(15 .. 16) := DCF77_Functions.Num_To_Str_L2(Ctx.Buf_Ctr);

		if C.Y = 0 or C.Y = 1 then
			L1(C.X + 1 .. C.X + 3) := Ctx.Get_State;
		else
			L2(C.X + 1 .. C.X + 3) := Ctx.Get_State;
		end if;
	end Draw_Bits_State;

	procedure Draw_Bits_Oszi(Ctx: in Bitlayer; L1, L2: in out String) is
		function Sym_For(Val: in U32; Has: in Boolean) return Character
				is (if Has then (if Val = 0 then '_' else '*')
					else ' ');
		Pos:     Integer := 31;
		Mask:    U32     := 16#8000_0000#;
		Raw:     U32;
		Flt:     U32;
		Has_Raw: Boolean := False;
		Has_Flt: Boolean := False;
	begin
		while Pos >= 0 loop
			Raw := (Ctx.Last_Raw      and Mask);
			Flt := (Ctx.Last_Filtered and Mask);
			if Pos < 16 then
				L1(16 - Pos) := Sym_For(Raw, Has_Raw);
				L2(16 - Pos) := Sym_For(Flt, Has_Flt);
			end if;
			if Raw /= 0 then
				Has_Raw := True;
			end if;
			if Flt /= 0 then
				Has_Flt := True;
			end if;
			Pos  := Pos - 1;
			Mask := Next_M(Mask);
		end loop;
	end Draw_Bits_Oszi;

end DCF77_Bitlayer;
