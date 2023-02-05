#!/bin/bash

set -ex

ANDROID_NUM=26
ANDROID_PLATFORM=android-$ANDROID_NUM
NDK_HOME=/home/mirmik/Android/Sdk/ndk/22.1.7171670
OVR_HOME=/home/mirmik/src/ovr_sdk_mobile_1.50.0
ANDROID_HOME=/home/mirmik/Android/Sdk
#JAVACPATH=/home/mirmik/soft/android-studio/jre/bin/javac
JAVACPATH=/usr/lib/jvm/java-8-openjdk-amd64/bin/javac
DEXPATH=$ANDROID_HOME/build-tools/30.0.3
COMPILERPATH=$NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin

rm -rf build
mkdir -p build
pushd build > /dev/null
$JAVACPATH\
	-classpath $ANDROID_HOME/platforms/$ANDROID_PLATFORM/android.jar\
	-d .\
	../src/main/java/com/makepad/hello_quest/*.java
$DEXPATH/dx --dex --output classes.dex .
mkdir -p lib/arm64-v8a
pushd lib/arm64-v8a > /dev/null

$COMPILERPATH/aarch64-linux-android$ANDROID_NUM-clang\
    -march=armv8-a\
    -fPIC\
    -shared \
    -std=c11\
    -I ~/project/rabbit\
    -I ~/project/nos\
    -I ~/project/igris\
    -I ~/project/ralgo\
    -I $NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/\
    -I $OVR_HOME/VrApi/Include\
    -L $NDK_HOME/platforms/$ANDROID_PLATFORM/arch-arm64/usr/lib\
    -L $OVR_HOME/VrApi/Libs/Android/arm64-v8a/Debug\
    -L /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/ \
    -L /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a \
    -o libmain_cc.so \
    -Wl,--start-group \
    -landroid\
    -llog\
    -lvrapi\
    -lGLESv3\
    -lEGL\
    -DWITH_OPENEXR=OFF -DBUILD_OPENEXR=OFF \
   ../../../src/main/cpp/*.c   \
   ~/project/igris/igris/osutil/realtime.c \
   ~/project/igris/igris/dprint/dprint_func_impl.c \
   ~/project/igris/igris/dprint/dprint_stdout.c \
   ~/project/igris/igris/string/replace_substrings.c \
   ~/project/igris/igris/string/memmem.c \
   /home/mirmik/Android/Sdk/ndk/22.1.7171670/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/$ANDROID_NUM/libc++.a \
    -Wl,--end-group 

$COMPILERPATH/aarch64-linux-android$ANDROID_NUM-clang\
    -march=armv8-a\
    -fPIC\
    -shared \
    -std=c++20\
    -I ~/project/rabbit\
    -I ~/project/nos\
    -I ~/project/igris\
    -I ~/project/ralgo\
    -I ~/project/morpheus\
    -I $NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/\
    -I $OVR_HOME/VrApi/Include\
    -L $NDK_HOME/platforms/$ANDROID_PLATFORM/arch-arm64/usr/lib\
    -L $OVR_HOME/VrApi/Libs/Android/arm64-v8a/Debug\
    -L . \
    -o libmain.so \
    -Wl,--start-group \
    -lEGL\
    -landroid\
    -llog\
    -lvrapi\
    -lGLESv3\
    -lmain_cc\
    -DWITH_OPENEXR=OFF -DBUILD_OPENEXR=OFF \
   ../../../src/main/cpp/*.cpp  \
   ~/project/rabbit/rabbit/opengl/drawer.cpp \
   ~/project/rabbit/rabbit/opengl/opengl_shader_program.cpp \
   ~/project/rabbit/rabbit/opengl/shader_collection.cpp \
   ~/project/rabbit/rabbit/mesh.cpp \
   ~/project/rabbit/rabbit/mesh/mesh.cpp \
   ~/project/rabbit/rabbit/font/naive.cpp \
   ~/project/igris/igris/sync/syslock_mutex.cpp \
   ~/project/igris/igris/osutil/src/posix.cpp \
   ~/project/igris/igris/util/string.cpp \
   ~/project/igris/igris/osinter/wait-linux.cpp \
   ~/project/igris/igris/string/replace.cpp \
    ~/project/nos/nos/print/print.cpp \
    ~/project/nos/nos/print/stdtype.cpp \
    ~/project/nos/nos/fprint/fprint.cpp \
    ~/project/nos/nos/fprint/fstdtype.cpp \
    ~/project/nos/nos/util/nos_numconvert.cpp \
    ~/project/nos/nos/io/ostream.cpp \
    ~/project/nos/nos/io/istream.cpp \
    ~/project/nos/nos/trent/trent.cpp \
    ~/project/nos/nos/trent/json.cpp \
    ~/project/nos/nos/input/input.cpp \
    ~/project/nos/nos/io/impls/current_ostream_stdout.cpp \
    ~/project/nos/nos/io/stdfile.cpp \
    ~/project/nos/nos/io/file.cpp \
    ~/project/nos/nos/inet/tcp_client.cpp \
    ~/project/nos/nos/inet/tcp_server.cpp \
    ~/project/nos/nos/inet/tcp_socket.cpp \
    ~/project/nos/nos/inet/udp_socket.cpp \
    ~/project/nos/nos/inet/common.cpp \
    ~/project/nos/nos/util/string.cpp \
    ~/project/nos/nos/util/osutil_unix.cpp \
    ~/project/morpheus/morpheus/ModelServer.cpp \
   /home/mirmik/Android/Sdk/ndk/22.1.7171670/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/$ANDROID_NUM/libc++.a \
    -Wl,--end-group 

    #-lopencv_core \
    #-lopencv_imgcodecs \
    #-lopencv_imgproc \
    #-llibtiff \
    #-llibopenjp2 \
    #-littnotify \
    #-ltbb \
    #-llibjpeg-turbo \
    #-ltegra_hal \
   # -lIlmImf \
  #  -llibwebp \
 #   -lz \
#    -llibpng \

cp $OVR_HOME/VrApi/Libs/Android/arm64-v8a/Debug/libvrapi.so .
popd > /dev/null
aapt\
	package\
	-F hello_quest.apk\
	-I $ANDROID_HOME/platforms/$ANDROID_PLATFORM/android.jar\
	-M ../src/main/AndroidManifest.xml\
	-f
aapt add hello_quest.apk classes.dex
aapt add hello_quest.apk lib/arm64-v8a/libmain_cc.so
aapt add hello_quest.apk lib/arm64-v8a/libmain.so
aapt add hello_quest.apk lib/arm64-v8a/libvrapi.so
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/libopencv_core.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/libopencv_imgcodecs.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/libopencv_imgproc.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibopenjp2.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libittnotify.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libtbb.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibjpeg-turbo.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libtegra_hal.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libIlmImf.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibwebp.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibpng.a
#aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibtiff.a

apksigner\
	sign\
	-ks ~/.android/debug.keystore\
	--ks-key-alias androiddebugkey\
	--ks-pass pass:android\
	hello_quest.apk
popd > /dev/null
