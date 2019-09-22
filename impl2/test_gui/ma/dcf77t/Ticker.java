package ma.dcf77t;

class Ticker extends Thread {

	private final Signal announceTick;
	private final Object monSyncTick = new Object();
	private boolean isTickingAutomatically = true;
	private int scaleRepeatMs = 1;

	Ticker(Signal announceTick) {
		super();
		this.announceTick = announceTick;
	}

	@Override
	public void run() {
		long nextTick = System.currentTimeMillis() + scaleRepeatMs;
		while(!isInterrupted()) {
			long curt = System.currentTimeMillis();
			long dly = nextTick - curt;
			nextTick = curt + scaleRepeatMs;
			try {
				if(dly > 0)
					sleep(dly);
			} catch(InterruptedException ex) {
				// harmless
				ex.printStackTrace();
				return;
			}

			if(isTickingAutomatically) {
				synchronized(monSyncTick) {
					monSyncTick.notifyAll();
				}
				announceTick.signal();
			}
		}
	}

	void tickManually() {
		announceTick.signal();
	}

	void setAutomaticTicking(boolean tick) {
		this.isTickingAutomatically = tick;
	}

	void setScale(int scaleRepeatMs) {
		this.scaleRepeatMs = scaleRepeatMs;
	}

	void delay(int ms) {
		for(int i = 0; i < ms; i++) {
			// The timeout is just in case it does not work the
			// regular way.
			try {
				synchronized(monSyncTick) {
					monSyncTick.wait(scaleRepeatMs + 1);
				}
			} catch(InterruptedException ex) {
				ex.printStackTrace(); // harmless
				return;
			}
		}
	}

}
