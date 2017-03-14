all: Test1.class Test2.class

Test1.class: Test1.java
	/usr/lib/jvm/jdk1.8.0_65/bin/javac Test1.java

Test2.class: Test2.java
	/usr/lib/jvm/jdk1.8.0_65/bin/javac Test2.java

run-release: Test1.class Test2.class
	$(HOME)/mercurial/jdk8/build/linux-x86_64-normal-server-release/jdk/bin/java Test2

run-debug: Test1.class Test2.class
	$(HOME)/mercurial/jdk8/build/linux-x86_64-normal-server-slowdebug/jdk/bin/java Test2	

clean:
	rm *.class
