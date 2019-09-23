#include "ll_out_display.h"
#include "ll_out_buzzer.h"
#include "ll_delay.h"
#include "display.h"

/* periodic beeping buzzer test */
int main(int argc, char** argv)
{
	char buzz = 1;
	struct display_ctx ctx_disp;

	ll_out_display_init();
	ll_out_buzzer_init();

	display_init_ctx(&ctx_disp);

	while(1) {
		ll_out_buzzer(buzz);
		ll_delay_ms(1000);
		buzz = !buzz;
	}
	return 0;
}
