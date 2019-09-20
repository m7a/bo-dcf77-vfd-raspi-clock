#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ll_delay.h"

void ll_delay_ms(unsigned num)
{
	char lbuf[256];
	printf("ll_delay_ms,%u\n", num);

	if(fgets(lbuf, sizeof(lbuf), stdin) == NULL) {
		puts("ERROR,Terminated by end of file on input.");
		exit(1);
	} else {
		if(strcmp(lbuf, "ACK,ll_delay_ms\n") != 0) {
			/* so we have a mismatch which is quite likely to
				mean: interrupt.
				TODO for now its not supported */
			;
		} /* else everything is OK / regular exit for this function */
	}
}
