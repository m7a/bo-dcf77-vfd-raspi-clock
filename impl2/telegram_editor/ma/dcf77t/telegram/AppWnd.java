package ma.dcf77t.telegram;

import java.util.Arrays;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

class AppWnd {

	private static final Color MEDIUMLIGHTGRAY = new Color(0xcc,0xcc,0xcc);
	private static final Color LIGHTBLUE = new Color(0xaa, 0xaa, 0xff);
	private static final Color LIGHTGREEN = new Color(0xaa, 0xff, 0xaa);
	private static final Color LIGHTYELLOW = new Color(0xff, 0xff, 0xaa);
	private static final Color LIGHTCYAN = new Color(0xaa, 0xff, 0xff);
	private static final Color LIGHTMAGENTA = new Color(0xff, 0xaa, 0xff);

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
	//               Anzahl der 1-en 29--36 muss gerade sein
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
	// 46    VAL     Monat EIner
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

	private final ExactTextField rawInOutBinary = new ExactTextField("",61);
	private final JTextArea rawInOutBinaryCSV = new JTextArea(3, 60);
	private final ExactTextField rawInOutHex = new ExactTextField("", 90);

	private final JFrame wnd;

	AppWnd() {
		super();
		wnd = new JFrame("Ma_Sys.ma DCF77 Telegram Editor");
		wnd.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		Container cp = wnd.getContentPane();
		cp.setLayout(new BorderLayout());

		JTabbedPane tabs = new JTabbedPane();
		Box telEditOuter = new Box(BoxLayout.Y_AXIS);
		JPanel telEditSpc = new JPanel(new FlowLayout(
						FlowLayout.CENTER, 10, 20));

		Box telegramEditor = new Box(BoxLayout.Y_AXIS);

		JPanel decodedRaw = new JPanel(new FlowLayout(FlowLayout.LEFT,
									0, 0));

		// -- Farben einstellen --
		field1weather.setBackground(MEDIUMLIGHTGRAY);
		field2call.setBackground(MEDIUMLIGHTGRAY);
		field3dstchg.setBackground(LIGHTBLUE);
		field4dstval.setBackground(LIGHTCYAN);
		field5leapsec.setBackground(LIGHTYELLOW);
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

		JPanel csvBinPan = new JPanel(new FlowLayout(
							FlowLayout.CENTER));
		rawInOutBinaryCSV.setLineWrap(true);
		csvBinPan.add(new JScrollPane(rawInOutBinaryCSV));
		csvBinPan.add(makeButton("Process Binary (0,1,3) CSV",
							this::processBinCSV));
		telEditOuter.add(csvBinPan);

		JPanel csvHexPan = new JPanel(new FlowLayout(
							FlowLayout.CENTER));
		csvHexPan.add(rawInOutHex);
		csvHexPan.add(makeButton("Process HEX (0xaa,...)",
							this::processHex));
		telEditOuter.add(csvHexPan);

		tabs.add("Telegram Details", telEditOuter);

		cp.add("Center", tabs);
		wnd.setLocation(50, 50);
		wnd.setSize(1200, 480); // fallback non-tiled wm
		wnd.setVisible(true);
	}

	private static JButton makeButton(String str, ActionListener lst) {
		JButton btn = new JButton(str);
		btn.addActionListener(lst);
		return btn;
	}

	private void showError(String msg) {
		JOptionPane.showMessageDialog(wnd, msg, "Error",
						JOptionPane.ERROR_MESSAGE);
	}

	private void processDecoded(ActionEvent ev) {
		// TODO MISSING
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

		// -- set raw in out binary csv --
		StringBuilder rv = new StringBuilder(
						String.valueOf(chr[0]));
		for(int i = 1; i < chr.length; i++) {
			rv.append(", ");
			rv.append(chr[i]);
		}
		rawInOutBinaryCSV.setText(rv.toString());

		// -- set raw in out hex --
		rv.setLength(0);
		for(int i = 0; i < chr.length; i += 4) {
			int val = 0;
			for(int j = 3; j >= 0; j--) {
				int assocBin;
				if(i + j >= chr.length) {
					assocBin = 1;
				} else {
					switch(chr[i + j]) {
					case '0': assocBin = 2; break; /* 10 */
					case '1': assocBin = 3; break; /* 11 */
					case '2': assocBin = 0; break; /* 00 */
					case '3': assocBin = 1; break; /* 01 */
					default: throw new RuntimeException(
						"N_IMPL: >" + chr[i + j] + "<");
					}
				}
				val = 0xff & ((val << 2) | assocBin);
			}
			rv.append(String.format("%02x,", val));
		}
		if(chr.length <= 60)
			rv.append("55,");
		rawInOutHex.setText(rv.toString());
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
				val = (val << 1) | ((chr[i] - '0') & 0xff);
			return String.valueOf(val);
		}
	}

	private void processBinCSV(ActionEvent ev) {
		rawInOutBinary.setText(rawInOutBinaryCSV.getText().
					replace(",", "").replace(" ", ""));
		processRaw();
	}

	private void processHex(ActionEvent ev) {
		String[] hexTokens = rawInOutHex.getText().split(",");
		int[] decodedTokens = new int[hexTokens.length];
		for(int i = 0; i < hexTokens.length; i++) {
			decodedTokens[i] = Integer.parseInt((hexTokens[i].
				startsWith("0x")? hexTokens[i].substring(2):
				hexTokens[i]).trim(), 16);
		}
		StringBuilder out = new StringBuilder();
		for(int i = 0; i < 61; i++) {
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

	// TODO CSTAT NEXT STEPS:
	// * NEED TO ADD SECOND HEX NOTATION AS OUTPUT-ONLY (with 0x... st. it can be used in arrays)
	// * Need to provide a real human datetime entry field with Timestamp "DD.MM.YYYY HH:ii:ss" + radiobutton CET/CEST + checkbox leap sec announce
	// * Need to perform and display parity checks

}

/*
Test vectors
010101010010111000101100100001000010110000111100000110100013
*/
