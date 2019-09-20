JAVA8  = /usr/lib/jvm/java-8-openjdk-amd64
JAVA11 = /usr/lib/jvm/java-11-openjdk-amd64

compile11:
	find ma -type f -name '*.java' -exec javac -Xlint:unchecked {} +

compile8:
	find ma -type f -name '*.java' -exec $(JAVA8)/bin/javac {} +

test11:
	$(JAVA11)/bin/java ma.dcf77t.Main

clean:
	find ma -type f -name '*.class' -delete
