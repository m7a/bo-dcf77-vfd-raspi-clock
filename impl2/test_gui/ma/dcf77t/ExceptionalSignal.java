package ma.dcf77t;

@FunctionalInterface
interface ExceptionalSignal<T extends Exception> {
	public void signal() throws T;
}
