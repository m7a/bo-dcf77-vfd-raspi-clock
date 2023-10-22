package ma.dcf77t;

class ChainedTries {

	private ChainedTries() {}

	static Exception tryThem(ExceptionalSignal... signals) {
		Exception myEx = new Exception("Chained tries exception.");
		for(ExceptionalSignal s: signals) {
			try {
				s.signal();
			} catch(Exception ex) {
				System.out.println("[INFO    ] Exception in " +
					"chained tries:" + ex.toString());
				myEx.addSuppressed(ex);
			}
		}
		return (myEx.getSuppressed().length == 0)? null: myEx;
	}

}
