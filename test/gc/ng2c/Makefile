all: Test1.class Test2.class

Test1.class: Test1.java
	javac Test1.java

Test2.class: Test2.java
	javac Test2.java

run-release: Test1.class Test2.class
	./launcher/ng2c-java Test2

run-debug: Test1.class Test2.class
	./launcher/ng2c-java-debug Test2

run-gdb: Test1.class Test2.class
	./launcher/ng2c-java-gdb Test2

clean:
	rm -f *.class *.log core
