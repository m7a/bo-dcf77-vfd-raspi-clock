package ma.dcf77t;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.function.Supplier;
import java.util.function.Consumer;

/** Abstract-away the gory details of the queue */
class LogTransferQueue implements Supplier<String>, Consumer<String> {

	private final ConcurrentLinkedQueue<String> q =
						new ConcurrentLinkedQueue<>();

	@Override
	public String get() {
		return q.poll();
	}

	@Override
	public void accept(String str) {
		q.add(str);
	}

}
