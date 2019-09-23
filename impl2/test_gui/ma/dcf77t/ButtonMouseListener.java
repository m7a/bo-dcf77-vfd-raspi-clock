package ma.dcf77t;

import java.awt.event.*;

class ButtonMouseListener extends MouseAdapter {

	private final Object[] origins;
	private final int[] assignedCodes;
	private final UserIOStatus userIn;

	ButtonMouseListener(Object[] origins, int[] assignedCodes,
							UserIOStatus userIn) {
		this.origins       = origins;
		this.assignedCodes = assignedCodes;
		this.userIn        = userIn;
		
	}

	@Override
	public void mousePressed(MouseEvent ev) {
		for(int i = 0; i < origins.length; i++) {
			if(ev.getSource() == origins[i]) {
				userIn.buttons = assignedCodes[i];
				return;
			}
		}
		// this is not expected to be reached
		System.out.println("[WARNING ] Unknown button pressed.");
	}

	@Override
	public void mouseReleased(MouseEvent ev) {
		userIn.buttons = 0;
	}

	@Override
	public void mouseExited(MouseEvent ev) {
		userIn.buttons = 0;
	}

}
