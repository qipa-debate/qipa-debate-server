
export JAVA_HOME=/path/to/your/jdk/

export CPLUS_INCLUDE_PATH=.:$JAVA_HOME/include:$JAVA_HOME/include/linux
export LD_LIBRARY_PATH=.
export PATH=${PATH}:./

1. build jar file
gradle build

2. build so file
g++ -Wall -shared RtcEngineImpl.cc VMUtil.cc -o libagora-rtc-server-jni.so -std=c++0x -L. -lvoip -fPIC -Wl,-z,defs

3. build test program
javac -classpath build/libs/agora-rtc-server-java-${version}.jar AgoraServerSDKTestor.java

4. run test program
java -classpath build/libs/agora-rtc-server-java-${version}.jar:. AgoraServerSDKTestor




