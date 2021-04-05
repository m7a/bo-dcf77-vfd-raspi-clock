package ma.dcf77t.telegram;

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.TitledBorder;
import java.text.SimpleDateFormat;
import java.text.ParseException;

// Test vector 010101010010111000101100100001000010110000111100000110100013

class AppWnd {

	private static final Color MEDIUMLIGHTGRAY = new Color(0x20, 0x20, 0x20);
	private static final Color LIGHTBLUE    = new Color(0x00, 0x00, 0x80);
	private static final Color LIGHTGREEN   = new Color(0x00, 0x80, 0x00);
	private static final Color LIGHTCYAN    = new Color(0x00, 0x80, 0x80);
	private static final Color LIGHTMAGENTA = new Color(0x80, 0x00, 0x80);
	private static final Color LIGHTYELLOW  = new Color(0x80, 0x80, 0x00);
	private static final Color LIGHTRED     = new Color(0x80, 0x00, 0x00);
	private static final Color CONSTCOLOR   = new Color(0x90, 0x90, 0x90);

	// 0
	private final PartialField field0const0  = new PartialField("0", 1);
	// 1--14 (14 items)
	private final PartialField field1weather = new PartialField(
							"______________", 14);
	// 15 Rufbit
	private final PartialField field2call    = new PartialField("_", 1);
	// 16    SCHALT  bei 1: Am Ende dieser Stunde Zeitumstellung!
	private final PartialField field3dstchg  = new PartialField("D", 1);
	// 17    SCHALT  bei 1: Sommerzeit, 0: Winterzeit \_ also 10 Sommerzeit
	// 18    SCHALT  bei 1: Winterzeit, 0: Sommerzeit /       01 Winterzeit
	private final PartialField field4dstval  = new PartialField("ST", 2);
	// 19    SCHALT  bei 1: Am Ende dieser Stunde Schaltsekunde!
	private final PartialField field5leapsec = new PartialField("_", 1);
	// 20    CONST1  Start Zeitinformationen 
	private final PartialField field6const1  = new PartialField("1", 1);
	// 21    VAL     Minute Einer   (1)  \_ müssen +1-Regel folgen, könnten
	// 22    VAL     Minute Einer   (2)  |  aber für 10min-Genauigkeit
	// 23    VAL     Minute Einer   (4)  |  erstmal ignoriert werden.
	// 24    VAL     Minute Einer   (8)  /
	private final PartialField field7mineiner = new PartialField("MinE", 4);
	// 25    VAL     Minute Zehner (10)
	// 26    VAL     Minute Zehner (20)
	// 27    VAL     Minute Zehner (40)
	private final PartialField field8minzehner = new PartialField("MZe", 3);
	// 28    CHCK    Parität Minute
	//               Anzahl der 1-en 21--28 muss gerade sein
	private final PartialField field9minpar    = new PartialField("_", 1);
	// 29    VAL     Stunde Einer   (1)
	// 30    VAL     Stunde Einer   (2)
	// 31    VAL     Stunde Einer   (4)
	// 32    VAL     Stunde Einer   (8)
	private final PartialField field10heiner = new PartialField("HEin", 4);
	// 33    VAL     Stunde Zehner (10)
	// 34    VAL     Stunde Zehner (20)
	private final PartialField field11hzehner = new PartialField("HZ", 2);
	// 35    CHCK    Parität Stunde
	//               Anzahl der 1-en 29--35 muss gerade sein
	private final PartialField field12hpar = new PartialField("_", 1);
	// 36    VAL     Tag Einer
	// 37    VAL     Tag Einer
	// 38    VAL     Tag Einer
	// 39    VAL     Tag Einer
	private final PartialField field13tageiner = new PartialField(
								"TEin", 4);
	// 40    VAL     Tag Zehner
	// 41    VAL     Tag Zehner
	private final PartialField field14tagzehner = new PartialField("TZ", 2);
	// 42    VAL     Wochentag (nur Einer von Mon1-Son7)
	// 43    VAL     Wochentag
	// 44    VAL     Wochentag
	private final PartialField field15dow = new PartialField("DOW", 3);
	// 45    VAL     Monat Einer
	// 46    VAL     Monat Einer
	// 47    VAL     Monat Einer
	// 48    VAL     Monat Einer
	private final PartialField field16moneiner = new PartialField(
								"MEin", 4);
	// 49    VAL     Monat Zehner (1 für +10, 0 für einstellige Monate)
	private final PartialField field17monzehner = new PartialField("Z", 1);
	// 50    VAL     Jahr Einer
	// 51    VAL     Jahr Einer
	// 52    VAL     Jahr Einer
	// 53    VAL     Jahr Einer
	private final PartialField field18jahreiner = new PartialField("JEin",
									4);
	// 54    VAL     Jahr Zehner
	// 55    VAL     Jahr Zehner
	// 56    VAL     Jahr Zehner
	// 57    VAL     Jahr Zehner
	private final PartialField field19jahrzehner = new PartialField("JZeh",
									4);
	// 58    CHCK    Parität Datum  Anzahl der 1-en 36-58 muss gerade sein
	private final PartialField field20datpar = new PartialField("_", 1);
	// 59    SPECIAL Endmarkierung -- kein Signal (3)
	private final PartialField field21ende = new PartialField("$", 1);

	private final PartialField[] fieldsRawDecoded = new PartialField[] {
		field0const0,
		field1weather,
		field2call,
		field3dstchg,
		field4dstval,
		field5leapsec,
		field6const1,
		field7mineiner,
		field8minzehner,
		field9minpar,
		field10heiner,
		field11hzehner,
		field12hpar,
		field13tageiner,
		field14tagzehner,
		field15dow,
		field16moneiner,
		field17monzehner,
		field18jahreiner,
		field19jahrzehner,
		field20datpar,
		field21ende,
	};

	private final ExactTextField parsedInOut = new ExactTextField("", 20);
	private final JRadioButton parsedCET = new JRadioButton("CET/Winter");
	private final JRadioButton parsedCEST = new JRadioButton("CEST/Summer");
	private final JCheckBox announceLeapSecond = new JCheckBox(
						"Leap Second Announced");

	private final ExactTextField rawInOutBinary = new ExactTextField("",61);
	private final JTextArea rawInOutBinaryCSV = new JTextArea(2, 60);
	private final JTextArea rawOutBinaryCSVBitlayer = new JTextArea(2, 60);
	private final ExactTextField rawInOutHex = new ExactTextField("", 80);
	private final ExactTextField rawInOutHexC = new ExactTextField("", 80);

	private final JTextArea statusText = new JTextArea(5, 60);

	private final JFrame wnd;

	AppWnd() {
		super();
		wnd = new JFrame("Ma_Sys.ma DCF77 Telegram Editor");
		wnd.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		Container cp = wnd.getContentPane();
		cp.setLayout(new BorderLayout());

		JTabbedPane tabs = new JTabbedPane();
		Box telEditOuter = new Box(BoxLayout.Y_AXIS);

		JPanel telEditParsed = new JPanel(new FlowLayout(
						FlowLayout.CENTER, 10, 20));
		telEditParsed.setBorder(new TitledBorder("Parsed"));
		telEditParsed.add(parsedInOut);
		ButtonGroup bg = new ButtonGroup();
		bg.add(parsedCET);
		bg.add(parsedCEST);
		telEditParsed.add(parsedCET);
		telEditParsed.add(parsedCEST);
		telEditParsed.add(announceLeapSecond);
		telEditParsed.add(makeButton("Process Parsed",
							this::processParsed));
		telEditOuter.add(telEditParsed);

		JPanel telEditSpc = new JPanel(new FlowLayout(
						FlowLayout.CENTER, 10, 20));
		telEditSpc.setBorder(new TitledBorder("Raw and Decoded"));

		Box telegramEditor = new Box(BoxLayout.Y_AXIS);

		JPanel decodedRaw = new JPanel(new FlowLayout(FlowLayout.LEFT,
									0, 0));

		// -- Farben einstellen --
		field0const0.setBackground(CONSTCOLOR);
		field1weather.setBackground(MEDIUMLIGHTGRAY);
		field2call.setBackground(MEDIUMLIGHTGRAY);
		field3dstchg.setBackground(LIGHTBLUE);
		field4dstval.setBackground(LIGHTCYAN);
		field5leapsec.setBackground(LIGHTYELLOW);
		field6const1.setBackground(CONSTCOLOR);
		field7mineiner.setBackground(LIGHTCYAN);
		field8minzehner.setBackground(LIGHTCYAN);
		field9minpar.setBackground(LIGHTYELLOW);
		field10heiner.setBackground(LIGHTCYAN);
		field11hzehner.setBackground(LIGHTCYAN);
		field12hpar.setBackground(LIGHTYELLOW);
		field13tageiner.setBackground(LIGHTCYAN);
		field14tagzehner.setBackground(LIGHTCYAN);
		field15dow.setBackground(LIGHTMAGENTA);
		field16moneiner.setBackground(LIGHTCYAN);
		field17monzehner.setBackground(LIGHTCYAN);
		field18jahreiner.setBackground(LIGHTMAGENTA);
		field19jahrzehner.setBackground(LIGHTMAGENTA);
		field20datpar.setBackground(LIGHTYELLOW);
		// -- Ende Farben --

		for(PartialField f: fieldsRawDecoded)
			decodedRaw.add(f);

		field0const0.setEnabled(false);
		field6const1.setEnabled(false);

		telegramEditor.add(decodedRaw);

		JPanel rawInOutBinaryPan = new JPanel(new FlowLayout(
							FlowLayout.LEFT, 0, 0));
		rawInOutBinaryPan.add(rawInOutBinary);
		telegramEditor.add(rawInOutBinaryPan);

		telEditSpc.add(telegramEditor);

		JPanel procPan = new JPanel(new GridLayout(2, 1));
		procPan.add(makeButton("Process Decoded",
							this::processDecoded));
		procPan.add(makeButton("Process Raw",
							this::processRaw));
		telEditSpc.add(procPan);

		telEditOuter.add(telEditSpc);

		JPanel csvBinPan = new JPanel(new BorderLayout());
		csvBinPan.setBorder(new TitledBorder("CSV Binary"));
		JPanel csvBinPanTxt = new JPanel(new GridLayout(2, 1));
		rawInOutBinaryCSV.setLineWrap(true);
		csvBinPanTxt.add(new JScrollPane(rawInOutBinaryCSV));
		rawOutBinaryCSVBitlayer.setLineWrap(true);
		rawOutBinaryCSVBitlayer.setEditable(false);
		csvBinPanTxt.add(new JScrollPane(rawOutBinaryCSVBitlayer));
		csvBinPan.add("Center", csvBinPanTxt);
		csvBinPan.add("East", makeButton("Process Binary (0,1,3) CSV",
							this::processBinCSV));
		telEditOuter.add(csvBinPan);

		JPanel csvHexPan = new JPanel(new FlowLayout(
							FlowLayout.CENTER));
		csvHexPan.setBorder(new TitledBorder("CSV Hex"));
		JPanel csvHexPanInner = new JPanel(new BorderLayout());
		Box csvLeft = new Box(BoxLayout.Y_AXIS);
		csvLeft.add(rawInOutHex);
		rawInOutHexC.setEditable(false);
		csvLeft.add(rawInOutHexC);
		csvHexPanInner.add("Center", csvLeft);
		csvHexPanInner.add("East", makeButton("Process HEX (ee/0xee)",
							this::processHex));
		csvHexPan.add(csvHexPanInner);
		telEditOuter.add(csvHexPan);

		JPanel statusPan = new JPanel(new BorderLayout());
		statusPan.setBorder(new TitledBorder("Status"));
		statusText.setEditable(false);
		statusText.setForeground(Color.RED);
		statusPan.add("Center", new JScrollPane(statusText));
		Box statusButtons = new Box(BoxLayout.Y_AXIS);
		statusButtons.add(makeButton("Reset", this::reset));
		statusPan.add("East", statusButtons);
		telEditOuter.add(statusPan);

		tabs.add("Telegram Details", telEditOuter);

		cp.add("Center", tabs);
		wnd.setLocation(50, 50);
		wnd.setSize(1200, 480); // fallback non-tiled wm
		wnd.setVisible(true);
	}

	private JButton makeButton(String str, ActionListener lst) {
		JButton btn = new JButton(str);
		btn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent ev) {
				statusText.setText("");
				try {
					lst.actionPerformed(ev);
				} catch(RuntimeException ex) {
					statusText.setText(ex.toString());
				}
			}
		});
		return btn;
	}

	private void showError(String msg) {
		JOptionPane.showMessageDialog(wnd, msg, "Error",
						JOptionPane.ERROR_MESSAGE);
	}

	private void processParsed(ActionEvent ev) {
		String text = parsedInOut.getText();

		SimpleDateFormat sdf = new SimpleDateFormat(
							"dd.MM.yyyy HH:mm:ss");
		TimeZone tz = TimeZone.getTimeZone("Europe/Berlin");
		sdf.setTimeZone(tz);
		Calendar cal = Calendar.getInstance(tz, Locale.GERMANY);
		try {
			cal.setTime(sdf.parse(text));
		} catch(ParseException ex) {
			throw new RuntimeException(ex);
		}
		int dow = transformToGermanDOWConvention(
						cal.get(Calendar.DAY_OF_WEEK));

		String[] tokens = text.split("[.: ]");

		setEinerZehner(tokens[0], field13tageiner, field14tagzehner);//d
		setEinerZehner(tokens[1], field16moneiner, field17monzehner);//m
		setEinerZehner(tokens[2].substring(2),
					field18jahreiner, field19jahrzehner);//Y
		setEinerZehner(tokens[3], field10heiner,  field11hzehner);   //h
		setEinerZehner(tokens[4], field7mineiner, field8minzehner);  //m

		field5leapsec.setText(announceLeapSecond.isSelected()?
								"1": "0");
		field4dstval.setText(parsedCET.isSelected()? "2": "1");

		field1weather.setText("00000000000000");
		field2call.setText("0");
		field3dstchg.setText("0");

		field9minpar.setText("0");
		field12hpar.setText("0");
		field20datpar.setText("0");
		field15dow.setText(String.valueOf(dow));

		field21ende.setText("3");

		processDecoded();

		// re-process on parity mismatch (quite inefficient...)
		if(correctParity(field9minpar) || correctParity(field12hpar) ||
						correctParity(field20datpar))
			processDecoded();
	}

	private static int transformToGermanDOWConvention(int us) {
		switch(us) {
		case Calendar.MONDAY:    return 1;
		case Calendar.TUESDAY:   return 2;
		case Calendar.WEDNESDAY: return 3;
		case Calendar.THURSDAY:  return 4;
		case Calendar.FRIDAY:    return 5;
		case Calendar.SATURDAY:  return 6;
		case Calendar.SUNDAY:    return 7;
		default: throw new RuntimeException("N_IMPL: " + us);
		}
	}

	/** @return true if value changed */
	private boolean correctParity(PartialField f) {
		if(f.getBackground() == LIGHTRED) {
			f.setText("1");
			return true;
		} else {
			return false;
		}
	}

	private void setEinerZehner(String data, PartialField e,
							PartialField z) {
		z.setText(String.valueOf(data.charAt(0)));
		e.setText(String.valueOf(data.charAt(1)));
	}

	private void processDecoded(ActionEvent ev) {
		processDecoded();
	}

	private void processDecoded() {
		char[] out = new char[60];
		int oidx = 0;
		for(int curf = 0; curf < fieldsRawDecoded.length; curf++) {
			int readbits = fieldsRawDecoded[curf].width;
			if(readbits == 1) {
				out[oidx] = fieldsRawDecoded[curf].getText().
								charAt(0);
			} else if(readbits > 4) {
				for(int j = 0; j < readbits; j++)
					out[oidx + j] = '0';
			} else {
				//String text = ;
				//int dataVal = (text.matches("[0-9]+"))?
				//		Integer.parseInt(text): 0;
				int dataVal = Integer.parseInt(
					fieldsRawDecoded[curf].getText());
				for(int j = 0; j < readbits; j++)
					out[oidx + j] = (((dataVal & (1 << j))
							!= 0)? '1': '0');
			}
			oidx += readbits;
		}
		rawInOutBinary.setText(new String(out));
		processRaw();
	}

	private void processRaw(ActionEvent ev) {
		processRaw();
	}

	private void processRaw() {
		char[] chr = rawInOutBinary.getText().trim().toCharArray();
		if(chr.length < 60) {
			showError("Input telegram too short.");
			return;
		}

		// -- set decoded --
		int curf = 0;
		int readbits = 0;
		for(int i = 0; i < chr.length &&
					curf < fieldsRawDecoded.length;) {
			readbits = fieldsRawDecoded[curf].width;
			String repr = decodeRawToHuman(chr, i, readbits);
			fieldsRawDecoded[curf].setText(repr);
			curf++;
			i += readbits;
		}

		// -- set human readable --
		parsedInOut.setText(String.format(
			"%s%s.%s%s.20%s%s %s%s:%s%s:00",
			field14tagzehner.getText(),  field13tageiner.getText(),
			field17monzehner.getText(),  field16moneiner.getText(),
			field19jahrzehner.getText(), field18jahreiner.getText(),
			field11hzehner.getText(),    field10heiner.getText(),
			field8minzehner.getText(),   field7mineiner.getText()
		));
		announceLeapSecond.setSelected(field5leapsec.getText().
								equals("1"));
		String dstval = field4dstval.getText();
		// inverse to what one would expect, but this is due to
		// endianess in decoding (this is not really a BCD field...)
		parsedCET.setSelected(dstval.equals("2"));
		parsedCEST.setSelected(dstval.equals("1"));

		// -- set raw in out binary csv --
		int parityMinuteOnes = 0; // sum bits 21-28
		int parityHourOnes   = 0; // sum bits 29-36
		int parityDateOnes   = 0; // sum bits 36-58

		StringBuilder rvc = new StringBuilder(
					String.valueOf(charToAssocBin(chr[0])));
		StringBuilder rv = new StringBuilder(
					String.valueOf(chr[0]));
		for(int i = 1; i < chr.length; i++) {
			rv.append(", ");
			rvc.append(", ");
			rv.append(chr[i]);
			rvc.append(charToAssocBin(chr[i]));

			if(chr[i] == '1') {
				if(i >= 21 && i <= 28)
					parityMinuteOnes++;
				if(i >= 29 && i <= 35)
					parityHourOnes++;
				if(i >= 36 && i <= 58)
					parityDateOnes++;
			}
		}
		rawInOutBinaryCSV.setText(rv.toString());
		rawOutBinaryCSVBitlayer.setText(rvc.toString());

		// -- set colors for parity check results --
		field9minpar.setBackground(((parityMinuteOnes % 2) == 0)?
							LIGHTYELLOW: LIGHTRED);
		field12hpar.setBackground(((parityHourOnes % 2) == 0)?
							LIGHTYELLOW: LIGHTRED);
		field20datpar.setBackground(((parityDateOnes % 2) == 0)?
							LIGHTYELLOW: LIGHTRED);

		// -- set raw in out hex --
		rvc.setLength(0);
		rv.setLength(0);
		for(int i = 0; i < chr.length; i += 4) {
			int val = 0;
			for(int j = 3; j >= 0; j--) {
				int assocBin = (i + j >= chr.length)? 1:
						charToAssocBin(chr[i + j]);
				val = 0xff & ((val << 2) | assocBin);
			}
			rv.append(String.format("%02x,", val));
			rvc.append(String.format("0x%02x,", val));
		}
		if(chr.length <= 60) {
			rv.append("55,");
			rvc.append("0x55,");
		}
		rawInOutHex.setText(rv.toString());
		rawInOutHexC.setText(rvc.toString());
	}

	private static int charToAssocBin(char in) {
		switch(in) {
		case '0': return 2; /* 10 */
		case '1': return 3; /* 11 */
		case '2': return 0; /* 00 */
		case '3': return 1; /* 01 */
		default: throw new RuntimeException("N_IMPL: >" + in + "<");
		}
	}

	private static String decodeRawToHuman(char[] chr, int offset,
								int length) {
		if(length > 4) {
			return new String(chr, offset, length); // return raw
		} else  {
			// decode BCD.
			// We do this for non-bcd things as well just for
			// maximum simplicity atm.
			int val = 0;
			for(int i = offset + length - 1; i >= offset; i--)
				val = (val << 1) |
						((chr[i] == '1'? 1: 0) & 0xff);
			return String.valueOf(val);
		}
	}

	private void processBinCSV(ActionEvent ev) {
		rawInOutBinary.setText(rawInOutBinaryCSV.getText().
					replace(",", "").replace(" ", ""));
		processRaw();
	}

	private void processHex(ActionEvent ev) {
		int i;
		String[] hexTokens = rawInOutHex.getText().split(",");
		int[] decodedTokens = new int[16];
		for(i = 0; i < hexTokens.length; i++) {
			decodedTokens[i] = Integer.parseInt((hexTokens[i].
				startsWith("0x")? hexTokens[i].substring(2):
				hexTokens[i]).trim(), 16);
		}
		for(; i < decodedTokens.length; i++) {
			decodedTokens[i] = 0;
		}
		StringBuilder out = new StringBuilder();
		for(i = 0; i < 61; i++) {
			switch(readEntry(decodedTokens[i / 4], i % 4)) {
			case 0: out.append("2"); break;
			case 1: out.append("3"); break;
			case 2: out.append("0"); break;
			case 3: out.append("1"); break;
			default: throw new RuntimeException("N_IMPL");
			}
		}
		rawInOutBinary.setText(out.toString());
		processRaw();
	}

	private static int readEntry(int in, int entry) {
		return (in & (3 << (entry * 2))) >> (entry * 2);
	}

	private void reset(ActionEvent ev) {
		for(PartialField f: fieldsRawDecoded)
			f.setText("");

		parsedInOut.setText("");
		parsedCET.setSelected(false);
		parsedCEST.setSelected(false);
		announceLeapSecond.setSelected(false);
		rawInOutBinary.setText("");
		rawInOutBinaryCSV.setText("");
		rawOutBinaryCSVBitlayer.setText("");
		rawInOutHex.setText("");
		statusText.setText("");
	}


}
