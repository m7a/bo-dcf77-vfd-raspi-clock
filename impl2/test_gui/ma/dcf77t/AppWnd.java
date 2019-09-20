package ma.dcf77t;

import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.*;
import javax.swing.border.TitledBorder;
import java.util.function.Supplier;

class AppWnd {

	// 20fps = 1000/20 = 50ms
	private static final int REPAINT_INTERVAL_MS = 50;

	private AppWnd() {}

	static void createAndShow(VirtualDisplay disp, final Signal onClose,
							Supplier<String> log) {
		final JFrame wnd = new JFrame("Ma_Sys.ma DCF-77 VFD Module " +
					"Clock Interactive Test Application");
		Container cp = wnd.getContentPane();
		cp.setLayout(new BorderLayout());

		// -- Virtual Clock --

		Box virtualClock = new Box(BoxLayout.Y_AXIS);

		JPanel displayPan = new JPanel(new FlowLayout(
							FlowLayout.CENTER));
		displayPan.setBorder(new TitledBorder("Virtual Display"));
		displayPan.add(disp);
		virtualClock.add(displayPan);

		Box clockFrontend = new Box(BoxLayout.X_AXIS);

		Box leftmost = new Box(BoxLayout.Y_AXIS);
		JPanel btnRst = new JPanel(new FlowLayout(FlowLayout.CENTER));
		JButton reset = new JButton("Reset"); // TODO dysfunctional
		btnRst.add(reset);
		leftmost.add(btnRst);
		JPanel buzpan = new JPanel(new FlowLayout(FlowLayout.CENTER));
		JLabel buzzer = new JLabel("Buzzer: ?"); // TODO dysfunctional
		buzpan.add(buzzer);
		leftmost.add(buzpan);
		leftmost.setBorder(new TitledBorder("Power+Buzzer"));
		clockFrontend.add(leftmost);

		JPanel middle = new JPanel(new BorderLayout());
		middle.add(BorderLayout.CENTER, RotationButton.create()); // TODO dysfunctional
		middle.setBorder(new TitledBorder("Mode Selection"));
		clockFrontend.add(middle);

		Box right = new Box(BoxLayout.Y_AXIS);
		JSlider light = new JSlider(); // TODO dysfunctional
		right.add(light);
		JPanel buttons = new JPanel(new FlowLayout(FlowLayout.CENTER));
		JButton b1 = new JButton("B1"); // TODO dysfunctional
		buttons.add(b1);
		JButton b2 = new JButton("B2");
		buttons.add(b2);
		right.add(buttons);
		right.setBorder(new TitledBorder("Light and Buttons"));
		clockFrontend.add(right);
		
		virtualClock.add(clockFrontend);

		cp.add(BorderLayout.EAST, virtualClock);

		// -- Analysis Unit --

		Box analysisUnit = new Box(BoxLayout.Y_AXIS);

		final JTextArea logArea = new JTextArea();
		logArea.setEditable(false);
		Box sub = new Box(BoxLayout.Y_AXIS);
		sub.add(new JScrollPane(logArea));
		sub.setBorder(new TitledBorder("Log"));
		analysisUnit.add(sub);

		cp.add(BorderLayout.CENTER, analysisUnit);

		// -- Other --

		final Timer timer = new Timer(REPAINT_INTERVAL_MS, __ -> {
			String newLogLine;
			while((newLogLine = log.get()) != null)
				logArea.append(newLogLine + "\n");
			disp.repaint();
		});

		wnd.setSize(1000, 600);
		wnd.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		wnd.addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent ev) {
				timer.stop();
				wnd.setVisible(false);
				onClose.signal();
			}
		});

		wnd.setVisible(true);
		timer.start();
	}

}
