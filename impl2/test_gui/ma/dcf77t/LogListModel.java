package ma.dcf77t;

import javax.swing.ListModel;
import javax.swing.event.ListDataListener;
import javax.swing.event.ListDataEvent;
import javax.swing.event.EventListenerList;

class LogListModel implements ListModel<String> {

	private static final int LOG_LINES = 500; // increase as needed/wanted

	private final EventListenerList listeners = new EventListenerList();
	private final String[] ringbuf = new String[LOG_LINES + 1];

	private int curOffset = 0;

	public void addLine(String line) {
		ringbuf[curOffset] = line;
		curOffset = (curOffset + 1) % ringbuf.length;
		for(ListDataListener l: listeners.getListeners(
						ListDataListener.class))
			l.contentsChanged(new ListDataEvent(this,
						ListDataEvent.CONTENTS_CHANGED,
						0, LOG_LINES - 1));
	}

	@Override
	public void addListDataListener(ListDataListener l) {
		listeners.add(ListDataListener.class, l);
	}

	@Override
	public String getElementAt(int index) {
		return ringbuf[(index + curOffset) % ringbuf.length];
	}

	@Override
	public int getSize() {
		return LOG_LINES;
	}

	@Override
	public void removeListDataListener(ListDataListener l) {
		listeners.remove(ListDataListener.class, l);
	}

}
