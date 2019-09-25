#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_proc_xeliminate.h"
#include "xeliminate_testcases.h"

static void run_xeliminate_testcases();

int main(int argc, char** argv)
{
	run_xeliminate_testcases();
}

static void run_xeliminate_testcases()
{
	unsigned char curtest;
	unsigned char i;
	unsigned char j;
	unsigned char bitval;
	unsigned char rv;
	unsigned char telegram[9][16];
	
	for(curtest = 0; curtest <
			(sizeof(xeliminate_testcases) /
			sizeof(struct xeliminate_testcase)); curtest++) {

		memset(telegram, 0, sizeof(telegram));

		for(i = 0; i < xeliminate_testcases[curtest].num_lines; i++) {
			for(j = 0; j <
			xeliminate_testcases[curtest].line_len[i]; j++) {
				switch(xeliminate_testcases[curtest].
								data[i][j]) {
				case 0:  bitval = DCF77_BIT_0;         break;
				case 1:  bitval = DCF77_BIT_1;         break;
				case 2:  bitval = DCF77_BIT_NO_UPDATE; break;
				case 3:  bitval = DCF77_BIT_NO_SIGNAL; break;
				default: puts("<<<ERROR1>>>"); exit(64);
				}
				telegram[i][j / 4] |= bitval << ((j % 4) * 2);
			}
			/*
			printf("Telegram %d:    ", i);
			for(j = 0; j < sizeof(telegram[0])/sizeof(unsigned char); j++)
				printf("%02x,", telegram[i][j]);
			puts("");
			*/
		}
		rv = 1;
		for(i = 1; rv == 1 && i < xeliminate_testcases[curtest].num_lines; i++)
			rv = dcf77_proc_xeliminate(
				xeliminate_testcases[curtest].line_len[i - 1],
				xeliminate_testcases[curtest].line_len[i],
				telegram[i - 1],
				telegram[i]
			);

		if(rv == xeliminate_testcases[curtest].recovery_ok) {
			if(rv == 1 && memcmp(telegram[xeliminate_testcases[curtest].num_lines - 1],
					xeliminate_testcases[curtest].recovers_to, 15) != 0) {
				/* fail */
				printf("[FAIL] Test %d: %s -- telegram mismatch\n",
					curtest, xeliminate_testcases[curtest].description);
				printf("       Expected  ");
				for(j = 0; j < 15; j++)
					printf(
						"%02x,",
						xeliminate_testcases[curtest].
						recovers_to[j]
					);
				printf("\n       Got       ");
				for(j = 0; j < 15; j++)
					printf(
						"%02x,",
						telegram[xeliminate_testcases[
						curtest].num_lines - 1][j]
					);
				puts("");
			} else {
				/* pass */
				printf("[ OK ] Test %d: %s\n", curtest,
					xeliminate_testcases[curtest].description);
			}
		} else {
			/* fail */
			printf("[FAIL] Test %d: %s -- unexpected rv idx=%d\n", curtest,
					xeliminate_testcases[curtest].description, i);
		}
		
	}
}
