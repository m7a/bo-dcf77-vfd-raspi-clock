with DCF77_Low_Level;

package DCF77VFD_OO is

	procedure Main;

private

	-- Must keep this as a singleton for access reasons
	LLI: aliased DCF77_Low_Level.LL;

end DCF77VFD_OO;
