package ma.dcf77t;

import java.util.function.BiConsumer;

@FunctionalInterface
interface SerialDisplayInterface extends BiConsumer<Boolean,Integer> {

	/**
	 * write function
	 * @param data is an unsigned byte 0..255
	 */
	void accept(Boolean isCtrl, Integer data);

}
