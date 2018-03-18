all: Test1.class Test2.class NG3.class

test = NG3

Test1.class: Test1.java
	javac Test1.java
	/home/rbruno/git/java-callgraph/process_test1.sh

Test2.class: Test2.java
	javac Test2.java
	/home/rbruno/git/java-callgraph/process_test2.sh

NG3.class: NG3.java
	javac NG3.java
	/home/rbruno/git/java-callgraph/process_ng3.sh

run-release: Test1.class Test2.class NG3.class
	./launcher/ng2c-java $(test)

run-debug: Test1.class Test2.class NG3.class
	./launcher/ng2c-java-debug $(test)

run-gdb: Test1.class Test2.class NG3.class
	./launcher/ng2c-java-gdb $(test)

clean:
	rm -f *.class *.log core
