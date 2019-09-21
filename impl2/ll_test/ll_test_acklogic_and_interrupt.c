#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "interrupt.h"
#include "ll_test_acklogic_and_interrupt.h"

static char pinval = 0;

void ll_test_acklogic_and_interrupt(char* expect)
{
	char lbuf[256];
	while(fgets(lbuf, sizeof(lbuf), stdin) != NULL) {
		if(strcmp(lbuf, expect) == 0) {
			/* regular exit for this function */
			return;
		} else if(strcmp(lbuf, "interrupt_service_routine,0\n") == 0) {
			pinval = 0;
			interrupt_service_routine();
			puts("ACK,interrupt_service_routine");
			fflush(stdout);
			/* continue */
		} else if(strcmp(lbuf, "interrupt_service_routine,1\n") == 0) {
			pinval = 1;
			interrupt_service_routine();
			puts("ACK,interrupt_service_routine");
			fflush(stdout);
			/* continue */
		} else {
			puts("ERROR,Unknown input from GUI.");
			fflush(stdout);
			return;
		}
	}

	/* got NULL */
	puts("ERROR,Terminated by end of file on input.");
	fflush(stdout);
	exit(1);
}

char ll_test_interrupt_read_pin()
{
	return pinval;
}
