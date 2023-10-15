/* parallel port interrupt demo */

#include <stdio.h>
#include <dos.h>
#include <time.h>

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
	/* disable(); */
	interflag++;
	/* printf("INTERRUPT RECIEVED %x %x %x\n", inportb(DATA),
				inportb(STATUS), inportb(CONTROL)); */
	outportb(picaddr, 0x20); /* Send End of Interrupt EOI */
	/* enable(); */
}

/* time in 10ms steps only w/ Borland and Microsoft compilers */
unsigned long clock2()
{
	union REGS regs;
	unsigned long c, s, m, h;
	regs.h.ah = 0x2c;
	intdos(&regs, &regs);
	c = regs.h.dl;
	s = regs.h.dh;
	m = regs.h.cl;
	h = regs.h.ch;
	return (c + 100 * (s + 60 * (m + 60 * h)));
}

int main()
{
	int c;
	int intno;   /* interrupt vector number */
	int picmask;

	unsigned k;
	unsigned unchanged;
	unsigned data, datan;
	unsigned stat, statn;
	unsigned ctrl, ctrln;
	unsigned long cstart;
	unsigned long cnow;

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
	puts("TODO INTERRUPT DISABLED");
	/* setvect(intno, parisr); */     /* set new interrupt vector entry */

	/* unmask PIC (TODO what is this about?) */
	outportb(picaddr + 1, inportb(picaddr + 1) & (0xff - picmask));

	/* enable parallel port IRQs */
	outportb(CONTROL, inportb(CONTROL) | 0x10);

	printf("IRQ %d, INTNO 0x%02x, PIC Addr 0x%x, Mask 0x02, TIME %lu\n",
				IRQ, intno, picaddr, picmask, time(NULL));

	interflag = 0; /* reset interrupt flag */

	/* getchar(); */
	/* delay(20000); */
	data = 0, datan = 0;
	stat = 0, statn = 0;
	ctrl = 0, ctrln = 0;
	cnow = clock2();
	cstart = cnow;
	unchanged = 0;
	for(k = 0; k < 2;) {
		datan = inportb(DATA);
		statn = inportb(STATUS);
		ctrln = inportb(CONTROL);

		if(interflag) {
			/*printf("GOT IFLAG %d, u = %d\n", interflag, unchanged);*/
			interflag = 0;
			/* unchanged = 0; */
		}
		if(data != datan || stat != statn || ctrl != ctrln) {
			/*
			if(unchanged > 130)
				printf("#");
			else
				printf(".");
			*/                  			fflush(stdout);
			if(unchanged < 80) {
				if(unchanged > 25)
					printf("#");
				else
					printf(".");
			} else if(unchanged > 350) {
				cnow = clock2();
				printf("\nNEW DATAGRAM (DELTA T = %lu)\n",
							cnow - cstart);
				cstart = cnow;
				k++;
			}
			/*
			printf("%x %x %x -> %x %x %x, u = %d\n",
					data, stat, ctrl,
					datan, statn, ctrln, unchanged);
			*/
			fflush(stdout);
			data = datan;
			stat = statn;
			ctrl = ctrln;
			unchanged = 0;
		} else {
			unchanged++;
		}

		delay(5);
		/* outportb(DATA, ~inportb(DATA)); */
	}

	/* disable parallel port IRQs */
	outportb(CONTROL, inportb(CONTROL) & 0xef);

	/* mask PIC */
	outportb(picaddr + 1, inportb(picaddr + 1) | picmask);

	setvect(intno, oldhandler); /* restore old interrupt handler */

	puts("PROGRAM FINISHED");
	getchar();
	return 0;
}