/* #include <stdio.h> */
#include <conio.h>
/* #include <dos.h> */

#define PORT 0x378

int main()
{
	puts("PROGRAM STARTED / ANY KEY TO EXIT");
	while(!kbhit()) {
		outportb(PORT, ~inportb(PORT));
		delay(1000);
	}
	puts("PROGRAM ENDED");
	/* getchar(); */
	return 0;
}