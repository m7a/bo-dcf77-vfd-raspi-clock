#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_proc_moventries.h"

int main(int argc, char** argv)
{
	struct dcf77_secondlayer test_ctx;

	dcf77_secondlayer_init(&test_ctx);
	/* TODO SET SOME VALUES, CALL PROCEDURE, COMPARE WITH EXPECTED VALUES */

	printf("Hello World\n");
	return EXIT_SUCCESS;
} 
