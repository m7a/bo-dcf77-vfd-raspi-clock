package ma.dcf77t;

class Ticker extends Thread implements ComProcOutRequestDelay {

	private static final int CURRENT_DELAY_NONE = -1;

	private final ComProcInQueueTickerSide toComProc;

	private boolean isTickingAutomatically = true;
	private int     currentDelay           = CURRENT_DELAY_NONE;
	private int     scaleRepeatMs          = 1;

	Ticker(ComProcInQueueTickerSide toComProc) {
		super();
		this.toComProc = toComProc;
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

			if(isTickingAutomatically)
				tick();
		}
	}

	void tick() {
		boolean delayCompleted;
		synchronized(this) {
			delayCompleted = (currentDelay != CURRENT_DELAY_NONE
							&& --currentDelay == 0);
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
				System.out.println("[WARN] Existing delay " +
							"cancelled/renewed.");
			currentDelay = (int)Math.max(ms, currentDelay);
		}
	}

}
