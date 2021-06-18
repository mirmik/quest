#!/bin/bash
NDK_HOME=/home/mirmik/Android/Sdk/ndk/22.1.7171670
OVR_HOME=/home/mirmik/src/ovr_sdk_mobile_1.44.0
ANDROID_HOME=/home/mirmik/Android/Sdk

JAVACPATH=/home/mirmik/soft/android-studio/jre/bin
DEXPATH=$ANDROID_HOME/build-tools/28.0.3
COMPILERPATH=$NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin

rm -rf build
mkdir -p build
pushd build > /dev/null
$JAVACPATH/javac\
	-classpath $ANDROID_HOME/platforms/android-26/android.jar\
	-d .\
	../src/main/java/com/makepad/hello_quest/*.java
$DEXPATH/dx --dex --output classes.dex .
mkdir -p lib/arm64-v8a
pushd lib/arm64-v8a > /dev/null
$COMPILERPATH/aarch64-linux-android26-clang\
    -march=armv8-a\
    -fPIC\
    -shared\
    -I ~/project/rabbit\
    -I ~/project/nos\
    -I ~/project/igris\
    -I ~/project/ralgo\
    -I $NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/\
    -I $OVR_HOME/VrApi/Include\
    -L $NDK_HOME/platforms/android-26/arch-arm64/usr/lib\
    -L $OVR_HOME/VrApi/Libs/Android/arm64-v8a/Debug\
    -landroid\
    -llog\
    -lvrapi\
    -lGLESv3\
    -lEGL\
    -o libmain.so\
   ../../../src/main/cpp/*.c   \
   ../../../src/main/cpp/*.cpp  \
   ~/project/rabbit/rabbit/opengl/drawer.cpp \
   ~/project/rabbit/rabbit/opengl/opengl_shader_program.cpp \
   ~/project/rabbit/rabbit/opengl/shader_collection.cpp \
   ~/project/rabbit/rabbit/mesh.cpp \
   ~/project/rabbit/rabbit/font/naive.cpp \
   ~/project/rabbit/rabbit/space/pose3.cpp \
   /home/mirmik/Android/Sdk/ndk/22.1.7171670/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/26/libc++.a




cp $OVR_HOME/VrApi/Libs/Android/arm64-v8a/Debug/libvrapi.so .
popd > /dev/null
aapt\
	package\
	-F hello_quest.apk\
	-I $ANDROID_HOME/platforms/android-26/android.jar\
	-M ../src/main/AndroidManifest.xml\
	-f
aapt add hello_quest.apk classes.dex
aapt add hello_quest.apk lib/arm64-v8a/libmain.so
aapt add hello_quest.apk lib/arm64-v8a/libvrapi.so

apksigner\
	sign\
	-ks ~/.android/debug.keystore\
	--ks-key-alias androiddebugkey\
	--ks-pass pass:android\
	hello_quest.apk
popd > /dev/null
