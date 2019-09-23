package ma.dcf77t;

import java.io.IOException;

class Ticker extends Thread implements ComProcOutRequestDelay {

	        static final int MS_PER_TICK        = 8;
	private static final int CURRENT_DELAY_NONE = 0;

	private final ComProcInQueueTickerSide toComProc;
	private final DCF77Sim                 dcf77;

	private boolean isTickingAutomatically = true;
	private int     currentDelay           = CURRENT_DELAY_NONE;

	/**
	 * Simulation Performance Considerations
	 * Without great issues, this simualtion can go up to 8x as fast as
	 * normal with 1ms sleep instead of 8ms between interupts. Afterwards,
	 * the interrupts become too frequent for the underlying program to
	 * actually progress and the system halts. This problem can (only?)
	 * be solved by limiting the occurrence of interrupts to sleeps which
	 * would then mean: A sleep incurs a certain number of interrupts
	 * dependent on its time. The system would then be fully snyhcornous
	 * s.t. the program could progress independent of the simulation
	 * speed. For now, we are contempt with 8x speed achievable by
	 * specifying a scale of 0.125f here.
	 */
	private float scaleRepeatMs = 1f;

	Ticker(ComProcInQueueTickerSide toComProc) throws IOException {
		super("Ticker");
		this.toComProc = toComProc;
		this.dcf77     = new DCF77Sim();
	}

	@Override
	public void run() {
		long nextTick = System.currentTimeMillis() +
				(int)Math.round(scaleRepeatMs * MS_PER_TICK);
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
				(int)Math.round(scaleRepeatMs * MS_PER_TICK);

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

		toComProc.sendTickToComProc(dcf77.tick());
	}

	void setAutomaticTicking(boolean tick) {
		this.isTickingAutomatically = tick;
	}

	void setScale(float scaleRepeatMs) {
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
