#include <stdio.h>

static unsigned char read_entry(unsigned char in, unsigned char entry)
{
	return (in & (3 << (entry * 2))) >> (entry * 2);
}

#define VAL_EPSILON 0 /* 00 */
#define VAL_X       1 /* 01 */
#define VAL_0       2 /* 10 */
#define VAL_1       3 /* 11 */

int main(int argc, char** argv)
{
	char in[15];
	int i;
	while(!feof(stdin)) {
		printf("> ");
		if(scanf("%02hhx,%02hhx,%02hhx,%02hhx,%02hhx,%02hhx,%02hhx,"
				"%02hhx,%02hhx,%02hhx,%02hhx,%02hhx,%02hhx,"
				"%02hhx,%02hhx,", in+0, in+1, in+2, in+3, in+4,
				in+5, in+6, in+7, in+8, in+9, in+10,
				in+11, in+12,in+13, in+14) != 15) {
			putchar('\n');
			return 0;
		}
		for(i = 0; i < 61; i++) {
			switch(read_entry(in[i / 4], i % 4)) {
			case VAL_EPSILON: printf("2,"); break;
			case VAL_X:       printf("3,"); break;
			case VAL_0:       printf("0,"); break;
			case VAL_1:       printf("1,"); break;
			default: printf("<<<ERROR>>>\n"); return 64;
			}
		}
		putchar('\n');
	}

	return 0;
}
