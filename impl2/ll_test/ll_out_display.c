#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ll_display.h"
#include "ll_out_display.h"

void ll_out_display(char is_ctrl, unsigned char value)
{
	char lbuf[256];
	printf("ll_out_display,%d,%u\n", is_ctrl, value);


	if(fgets(lbuf, sizeof(lbuf), stdin) == NULL) {
		puts("ERROR,Terminated by end of file on input.");
		exit(1);
	} else {
		if(strcmp(lbuf, "ACK,ll_out_display\n") != 0) {
			/* so we have a mismatch which is quite likely to
				mean: interrupt.
				TODO for now its not supported */
			;
		} /* else everything is OK / regular exit for this function */
	}
}
