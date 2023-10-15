package ma.dcf77t;

import java.awt.event.*;

class ButtonMouseListener extends MouseAdapter {

	private final Object[] origins;
	private final String[] assignedLabels;
	private final UserIOStatus userIn;

	ButtonMouseListener(Object[] origins, String[] assignedLabels,
							UserIOStatus userIn) {
		this.origins        = origins;
		this.assignedLabels = assignedLabels;
		this.userIn         = userIn;
		
	}

	@Override
	public void mousePressed(MouseEvent ev) {
		for(int i = 0; i < origins.length; i++) {
			if(ev.getSource() == origins[i]) {
				userIn.buttons = assignedLabels[i];
				return;
			}
		}
		// this is not expected to be reached
		System.out.println("[WARNING ] Unknown button pressed.");
	}

	@Override
	public void mouseReleased(MouseEvent ev) {
		userIn.buttons = null;
	}

	@Override
	public void mouseExited(MouseEvent ev) {
		userIn.buttons = null;
	}

}
