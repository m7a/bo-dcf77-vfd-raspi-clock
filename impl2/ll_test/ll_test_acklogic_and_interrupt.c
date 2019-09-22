#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Unix dependent for nonblocking IO */
#include <unistd.h>
#include <fcntl.h>

#include "interrupt.h"
#include "ll_test_acklogic_and_interrupt.h"

static char check_and_proc_isr(char* lbuf);

void ll_test_acklogic_and_interrupt(char* expect)
{
	char lbuf[256];
	while(fgets(lbuf, sizeof(lbuf), stdin) != NULL) {
		if(strcmp(lbuf, expect) == 0) {
			/* regular exit for this function */
			return;
		} else if(!check_and_proc_isr(lbuf)) {
			puts("ERROR,Unknown input from GUI.");
			fflush(stdout);
			return;
			/* otherwise continue */
		}
	}

	/* got NULL */
	puts("ERROR,Terminated by end of file on input.");
	fflush(stdout);
	exit(1);
}

/* @return 0 if nothing processed, 1 if processed */
static char check_and_proc_isr(char* lbuf)
{
	if(strcmp(lbuf, "interrupt_service_routine,0\n") == 0) {
		interrupt_service_routine(0);
		puts("ACK,interrupt_service_routine");
		fflush(stdout);
		return 1;
	} else if(strcmp(lbuf, "interrupt_service_routine,1\n") == 0) {
		interrupt_service_routine(1);
		puts("ACK,interrupt_service_routine");
		fflush(stdout);
		return 1;
	} else {
		return 0;
	}
}

void ll_test_handle_pending_interrupts()
{
	char lbuf[256];
	int bakfcntl = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, bakfcntl | O_NONBLOCK);
	while(fgets(lbuf, sizeof(lbuf), stdin) != NULL) {
		if(!check_and_proc_isr(lbuf)) {
			puts("ERROR,Mismatch on C side.");
			fflush(stdout);
			break;
		}
	}
	fcntl(0, F_SETFL, bakfcntl);
}
