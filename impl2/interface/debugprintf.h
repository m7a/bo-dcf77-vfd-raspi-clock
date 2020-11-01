#ifdef DEBUG
#	include <stdio.h>
#	define DEBUGPRINTF printf
#else
#	define DEBUGPRINTF(X, ...) {}
#endif
