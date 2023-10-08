with Ada.Text_IO;
use  Ada.Text_IO;

with DCF77_Low_Level;

procedure DCF77Simul is
begin

	while not Is_EOF loop
		declare
			Line: constant String := Get_Line;
		begin
			null; -- TODO ... REPL HERE
		end;
	end loop;

end DCF77Simul;
