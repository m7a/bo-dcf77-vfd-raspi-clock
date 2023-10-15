#include <util/delay.h>

#include "ll_delay.h"

void ll_delay_ms(unsigned num)
{
	unsigned i;
	for(i = 0; i < num; i++)
		_delay_ms(1);
}
