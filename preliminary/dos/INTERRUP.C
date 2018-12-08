/* parallel port interrupt demo */

#include <stdio.h>
#include <dos.h>

#define PORTADDRESS 0x378
#define IRQ         7

#define DATA    (PORTADDRESS + 0)
#define STATUS  (PORTADDRESS + 1)
#define CONTROL (PORTADDRESS + 2)

#define PIC1 0x20
#define PIC2 0xA0

int interflag;
int picaddr; /* programmable interrupt controller (PIC) base addresse */

void interrupt (*oldhandler)(); /* backup function pointer of old handler */

void interrupt parisr() /* interrupt service routine ISR */
{
	interflag = 1;
	puts("INTERRUPT RECIEVED");
	outportb(picaddr, 0x20); /* Send End of Interrupt EOI */
}

int main()
{
	int c;
	int intno;   /* interrupt vector number */
	int picmask;

	if(IRQ >= 2 && IRQ <= 7) {
		intno = IRQ + 0x08;
		picaddr = PIC1;
		picmask = 1 << IRQ;
	} else if (IRQ >= 8 && IRQ <= 15) {
		intno = IRQ + 0x68;
		picaddr = PIC2;
		picmask = 1 << (IRQ - 8);
	} else {
		fputs("IRQ out of range\n", stderr);
		return 2;
	}

	/* ensure port is in forward direction */
	outportb(CONTROL, inportb(CONTROL) & 0xdf);

	outportb(DATA, 0xff); /* turn all data bits on */

	oldhandler = getvect(intno); /* backup old interrupt vector */
	setvect(intno, parisr);      /* set new interrupt vector entry */

	/* unmask PIC (TODO what is this about?) */
	outportb(picaddr + 1, inportb(picaddr + 1) & (0xff - picmask));

	/* enable parallel port IRQs */
	outportb(CONTROL, inportb(CONTROL) | 0x10);

	printf("IRQ %d, INTNO %02x, PIC Addr 0x%x, Mask 0x02x\n",
					IRQ, intno, picaddr, picmask);

	interflag = 0; /* reset interrupt flag */
	delay(10);
	outportb(DATA, 0x00); /* high to low transition */
	delay(10);

	if(interflag == 1) {
		puts("Interrupts occur on high to low transition of ACK");
	} else {
		outportb(DATA, 0xff); /* low to high transition */
		delay(10);
		if(interflag == 1)
			puts("Interrupts occur on low to high "
						"transition of ACK");
		else
			puts("NO INTERRUPT ACTIVITY DETECTED");
	}

	/* disable parallel port IRQs */
	outportb(CONTROL, inportb(CONTROL) & 0xef);

	/* mask PIC */
	outportb(picaddr + 1, inportb(picaddr + 1) | picmask);

	setvect(intno, oldhandler); /* restore old interrupt handler */

	getchar();
	return 0;
}