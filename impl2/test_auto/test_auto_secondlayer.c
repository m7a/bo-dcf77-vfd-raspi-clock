#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dcf77_bitlayer.h"
#include "dcf77_secondlayer.h"
#include "dcf77_secondlayer_xeliminate.h"
#include "dcf77_secondlayer_moventries.h"
#include "dcf77_secondlayer_recompute_eom.h"
#include "dcf77_secondlayer_process_telegrams.h"
#include "dcf77_telegram.h"
#include "inc_sat.h"
#include "xeliminate_testcases.h"

/* TODO z once we have reorganization: do a test with a leap second (like test case 9) but "miss" the leap second in the sense that just a 3/NO_SIGNAL is output for the 0-marker in the leap second. It will be interesting to see how long it takes to recover from that! */

/* -------------------------------------------------[ Test Implementation ]-- */

static void printtel(unsigned char* data, unsigned char bitlen);
static void printtel_sub(unsigned char* data);
static void dumpmem(struct dcf77_secondlayer* ctx);

static const unsigned char CMPMASK[16] = {
0x03,0x00,0x00,0x00,0x00,0x03,0x00,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

int main(int argc, char** argv)
{
	struct dcf77_secondlayer uut;

	unsigned char pass;
	unsigned char cmpbuf[DCF77_SECONDLAYER_LINE_BYTES];

	unsigned char curtest;
	unsigned char i;
	unsigned char j;
	unsigned char bitval;
	
	for(curtest = 0; curtest <
			(sizeof(xeliminate_testcases) /
			sizeof(struct xeliminate_testcase)); curtest++) {

		/* for now skip tests which fail recovery */
		if(!xeliminate_testcases[curtest].secondlayer_required &&
				xeliminate_testcases[curtest].recovery_ok == 0)
			continue;

		puts("======================================================="
						"=========================");
		printf("Test case %u: %s\n", curtest,
				xeliminate_testcases[curtest].description);
		/*
		 * we initialize everything to 0 to avoid data from the previous
		 * tests to be present. TODO z might want to try to disable this
		 * once the test runs automatically 
		 */
		memset(&uut, 0, sizeof(struct dcf77_secondlayer));
		dcf77_secondlayer_init(&uut);

		for(i = 0; i < xeliminate_testcases[curtest].num_lines; i++) {
			printf("  Line %2u -----------------------------------"
				"-----------------------------------\n", i);
			for(j = 0; j <
			xeliminate_testcases[curtest].line_len[i]; j++) {
				switch(xeliminate_testcases[curtest].
								data[i][j]) {
				case 0:  bitval = DCF77_BIT_0;         break;
				case 1:  bitval = DCF77_BIT_1;         break;
				case 2:  bitval = DCF77_BIT_NO_UPDATE; break;
				case 3:  bitval = DCF77_BIT_NO_SIGNAL; break;
				default: puts("    *** ERROR1 ***"); exit(64);
				}

				uut.in_val = bitval;
				dcf77_secondlayer_process(&uut);
				if(0 == 1) /* currently disabled */
					dumpmem(&uut);
				if(uut.out_telegram_1_len != 0) {
					printf("    out1len=%u out2len=%u\n",
							uut.out_telegram_1_len,
							uut.out_telegram_2_len);
					printtel(uut.out_telegram_1,
							uut.out_telegram_1_len);
					printtel(uut.out_telegram_2,
							uut.out_telegram_2_len);

					memcpy(cmpbuf, uut.out_telegram_1,
						DCF77_SECONDLAYER_LINE_BYTES);

					uut.out_telegram_1_len = 0;
					uut.out_telegram_2_len = 0;
				}
			}
		}
		/* now compare */
		pass = 1;
		for(i = 0; i < DCF77_SECONDLAYER_LINE_BYTES; i++) {
			if((cmpbuf[i] & CMPMASK[i]) != (xeliminate_testcases[
					curtest].recovers_to[i] & CMPMASK[i])) {
				printf("  [FAIL] Mismatch at index %d\n", i);
				printtel(cmpbuf, 60);
				pass = 0;
				break;
			}
		}
		/*
		 * Note that this test is not very prcise, but if it fails, it
		 * is quite likely that there is something amiss.
		 */
		if(pass)
			printf("  [ OK ] Matches wrt. CMPMASK\n");
	}
}

static void printtel(unsigned char* data, unsigned char bitlen)
{
	printf("    tellen=%2u val=", bitlen);
	printtel_sub(data);
}

static void printtel_sub(unsigned char* data)
{
	unsigned char j;
	for(j = 0; j < 15; j++)
		printf("%02x,", data[j]);
	putchar('\n');
}

static void dumpmem(struct dcf77_secondlayer* ctx)
{
	unsigned char i;
	printf("    [DEBUG]         ");
	for(i = 0; i < DCF77_SECONDLAYER_LINE_BYTES; i++) {
		if((ctx->private_line_cursor/4) == i) {
			if(ctx->private_line_cursor % 4 >= 2)
				printf("*  ");
			else
				printf(" * ");
		} else {
			printf("   ");
		}
	}
	putchar('\n');
	for(i = 0; i < DCF77_SECONDLAYER_LINES; i++) {
		printf("    [DEBUG] %s meml%d=", i == ctx->private_line_current?
								"*": " ", i);
		printtel_sub(ctx->private_telegram_data +
					(i * DCF77_SECONDLAYER_LINE_BYTES));
	}
	printf("    [DEBUG] line_current=%u, cursor=%u\n",
			ctx->private_line_current, ctx->private_line_cursor);
}
