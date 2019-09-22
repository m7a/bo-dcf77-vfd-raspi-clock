/* HW init */
void ll_input_init();

/*
 * No measurement evaluation so far.
 */
unsigned char ll_input_read_sensor();

/*
 * Button measurement evaluation
 * 
 * Measurement  Button          Safety PM  Interval
 *   0 +- 1     => none         +30        [  0; 30]
 * 127 +- 1     => 10k  / BTN2  6          [121;133]
 * 172 +- 3     => 4.7k / BTN1  8          [164;180]
 * 194 +- 3     => both         8          [186;202]
 */
unsigned char ll_input_read_buttons();

/*
 * No measurement evaluation so far
 */
unsigned char ll_input_read_mode();
