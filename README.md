# OpenJDK 8 HotSpot with the [NG2C](https://github.com/rodrigo-bruno/ng2c) and [ROLP](https://github.com/rodrigo-bruno/rolp)

[Check our paper at EuroSys'19](https://rodrigo-bruno.github.io/publications/rbruno-eurosys19.pdf)

In order to test ROLP you first need to download and prepare the OpenJDK JVM build. Instructions regarding how to download and build the OpenJDK can be found at: http://hg.openjdk.java.net/jdk8/jdk8/raw-file/tip/README-builds.html

Then you need to replace the HotSpot source with the one in this repository. For example:

    git clone https://github.com/rodrigo-bruno/rolp.git hotspot-rolp
    mv /path-to-openjdk/jdk8/hotspot /path-to-openjdk/jdk8/hotspot-bak
    ln -s hotspot-rolp /path-to-openjdk/jdk8/hotspot

Finally, build the OpenJDK JVM and the resulting JVM has NG2C+ROLP incorporated.
