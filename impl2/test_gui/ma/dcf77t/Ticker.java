package ma.dcf77t;

class Ticker extends Thread implements ComProcOutRequestDelay {

	private static final int MS_PER_TICK = 8;

	private static final int CURRENT_DELAY_NONE = 0;

	private final ComProcInQueueTickerSide toComProc;

	private boolean isTickingAutomatically = true;
	private int     currentDelay           = CURRENT_DELAY_NONE;
	private int     scaleRepeatMs          = 1;

	Ticker(ComProcInQueueTickerSide toComProc) {
		super("Ticker");
		this.toComProc = toComProc;
	}

	@Override
	public void run() {
		long nextTick = System.currentTimeMillis() +
						scaleRepeatMs * MS_PER_TICK;
		while(!isInterrupted()) {
			long dly = nextTick - System.currentTimeMillis();
			try {
				if(dly > 0)
					sleep(dly);
			} catch(InterruptedException ex) {
				System.out.println("[INFO    ] Ticker " +
						"shutdown due to " +
						"InterruptedException " +
						"(usually harmless).");
				return;
			}

			nextTick = System.currentTimeMillis() +
						scaleRepeatMs * MS_PER_TICK;

			if(isTickingAutomatically)
				tick();
		}
	}

	void tick() {
		boolean delayCompleted = false;
		synchronized(this) {
			if(currentDelay != CURRENT_DELAY_NONE) {
				currentDelay -= 8;
				if(currentDelay <= 0) {
					currentDelay = CURRENT_DELAY_NONE;
					delayCompleted = true;
				}
			}
		}
		if(delayCompleted)
			toComProc.sendDelayCompletedToComProc();
		toComProc.sendTickToComProc();
	}

	void setAutomaticTicking(boolean tick) {
		this.isTickingAutomatically = tick;
	}

	void setScale(int scaleRepeatMs) {
		this.scaleRepeatMs = scaleRepeatMs;
	}

	@Override
	public void accept(Integer ms) {
		synchronized(this) {
			if(currentDelay > 0)
				System.out.println("[WARNING ] Existing " +
						"delay cancelled/renewed: " +
						currentDelay);
			currentDelay = (int)Math.max(ms, currentDelay);
		}
	}

}
