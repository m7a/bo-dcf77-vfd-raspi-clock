#include <stdio.h>

int main(int argc, char** argv)
{
	unsigned char firsttel[] = { 14  , 0   , 193 , 32  , 0   , 199 , 18  , 16  , 130 , 35  , 175, 169, 240, 131, 123, };
	unsigned char secondtel[] = { 2   , 0   , 193 , 33  , 0   , 199 , 18  , 16  , 130 , 32  , 128, 160, 240, 131, 115, };

	for(int i = 0; i < sizeof(firsttel)/sizeof(unsigned char); i++) {
		printf("%02x,", firsttel[i]);
	}

	puts("");

	for(int i = 0; i < sizeof(secondtel)/sizeof(unsigned char); i++) {
		printf("%02x,", secondtel[i]);
	}

	puts("");
	return 0;
}
