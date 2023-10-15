package ma.dcf77t;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.plaf.metal.MetalLookAndFeel;
import javax.swing.plaf.metal.MetalTheme;
import javax.swing.border.TitledBorder;
import java.util.function.Supplier;

class AppWnd {

	// 20fps = 1000/20 = 50ms
	// 15fps = 1000/15 = 66.66ms
	private static final int REPAINT_INTERVAL_MS = 67;

	private AppWnd() {}

	static void createAndShow(VirtualDisplay disp, final Signal onClose,
			Supplier<String> log, final UserIOStatus userIn) {
		setDesign();

		final JFrame wnd = new JFrame("Ma_Sys.ma DCF-77 VFD Raspi " +
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
		JButton reset = new JButton("Reset");
		reset.setEnabled(false); // TODO z dysfunctional
		btnRst.add(reset);
		btnRst.setBorder(new TitledBorder("Power"));
		leftmost.add(btnRst);
		JPanel buzpan = new JPanel(new FlowLayout(FlowLayout.CENTER));
		final JLabel buzzer = new JLabel("?");
		buzpan.add(buzzer);
		buzpan.setBorder(new TitledBorder("Buzzer"));
		leftmost.add(buzpan);
		JPanel alledpan = new JPanel(new FlowLayout(FlowLayout.CENTER));
		final JLabel alarm = new JLabel("?");
		alledpan.add(alarm);
		alledpan.setBorder(new TitledBorder("Alarm LED"));
		leftmost.add(alledpan);
		clockFrontend.add(leftmost);

		Box right = new Box(BoxLayout.Y_AXIS);
		JSlider light = new JSlider(0, 255);
		light.addChangeListener((__) -> { userIn.light =
							light.getValue(); });
		light.setValue(0);
		right.add(light);
		JPanel buttons = new JPanel(new FlowLayout(FlowLayout.CENTER));
		JButton[] btn = {
			new JButton("Green"), new JButton("Left"),
			new JButton("Right"), new JButton("(L+R)"),
			new JButton("Red")
		};
		for (JButton b: btn)
			buttons.add(b);
		MouseListener listen = new ButtonMouseListener(
			new Object[] { btn[0], btn[1], btn[2], btn[3], btn[4] },
			new String[] { "green", "left", "right", "l+r", "red" },
			userIn
		);
		for (JButton b: btn)
			b.addMouseListener(listen);

		right.add(buttons);
		right.setBorder(new TitledBorder("Light and Buttons"));
		clockFrontend.add(right);
		
		virtualClock.add(clockFrontend);

		cp.add(BorderLayout.EAST, virtualClock);

		// -- Analysis Unit --

		Box analysisUnit = new Box(BoxLayout.Y_AXIS);

		final LogListModel logListData = new LogListModel();
		final JList<String> logList = new JList<>(logListData);
		logList.setCellRenderer(new LogListRenderer());
		logList.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
		Box sub = new Box(BoxLayout.Y_AXIS);
		sub.add(new JScrollPane(logList));
		sub.setBorder(new TitledBorder("Log"));
		analysisUnit.add(sub);

		cp.add(BorderLayout.CENTER, analysisUnit);

		// -- Other --

		final Timer timer = new Timer(REPAINT_INTERVAL_MS, __ -> {
			String newLogLine;
			while((newLogLine = log.get()) != null)
				logListData.addLine(newLogLine);
			
			buzzer.setText(userIn.buzzer?  "BUZZ!":  "no buzz");
			alarm.setText(userIn.alarmLED? "LIGHT!": "no light");
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

	private static void setDesign() {
		System.setProperty("sun.awt.noerasebackground", "true");
		Toolkit.getDefaultToolkit().setDynamicLayout(true);
		MetalTheme theme = new LabyrintheTheme();
		MetalLookAndFeel.setCurrentTheme(theme);
		try {
			UIManager.setLookAndFeel(new MetalLookAndFeel());
		} catch(Exception ex) {
			ex.printStackTrace();
		}
	}

}
