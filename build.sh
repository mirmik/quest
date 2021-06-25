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
    -shared \
    -I ~/project/rabbit\
    -I ~/project/nos\
    -I ~/project/igris\
    -I ~/project/ralgo\
    -I ~/project/crow\
    -I $NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/\
    -I $OVR_HOME/VrApi/Include\
    -I ~/Downloads/opencv-4.5.2/modules/imgcodecs/include/ \
    -I ~/Downloads/opencv-4.5.2/modules/core/include/ \
    -I ~/Downloads/opencv-4.5.2/include/ \
    -L $NDK_HOME/platforms/android-26/arch-arm64/usr/lib\
    -L $OVR_HOME/VrApi/Libs/Android/arm64-v8a/Debug\
    -L /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/ \
    -L /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a \
    -o libmain.so \
    -Wl,--start-group \
    -landroid\
    -llog\
    -lvrapi\
    -lGLESv3\
    -lopencv_core \
    -lopencv_imgcodecs \
    -lopencv_imgproc \
    -lEGL\
    -llibtiff \
    -llibopenjp2 \
    -littnotify \
    -ltbb \
    -llibjpeg-turbo \
    -ltegra_hal \
    -lIlmImf \
    -llibwebp \
    -lz \
    -llibpng \
    -DWITH_OPENEXR=OFF -DBUILD_OPENEXR=OFF \
   ../../../src/main/cpp/*.c   \
   ../../../src/main/cpp/*.cpp  \
   ~/project/rabbit/rabbit/opengl/drawer.cpp \
   ~/project/rabbit/rabbit/opengl/opengl_shader_program.cpp \
   ~/project/rabbit/rabbit/opengl/shader_collection.cpp \
   ~/project/rabbit/rabbit/mesh.cpp \
   ~/project/rabbit/rabbit/font/naive.cpp \
   ~/project/rabbit/rabbit/space/pose3.cpp \
   ~/project/crow/crow/src/stdtime.cpp \
   ~/project/crow/crow/src/print-stub.cpp \
   ~/project/crow/crow/src/warn-stub.cpp \
   ~/project/crow/crow/pubsub/pubsub.cpp \
   ~/project/crow/crow/src/tower.cpp \
   ~/project/crow/crow/select.cpp \
   ~/project/crow/crow/src/threads-posix.cpp \
   ~/project/crow/crow/src/gateway.cpp \
   ~/project/crow/crow/src/address.cpp \
   ~/project/crow/crow/src/hexer.c \
   ~/project/crow/crow/src/packet.cpp \
   ~/project/crow/crow/src/packet_ptr.cpp \
   ~/project/crow/crow/src/allocation_malloc.cpp \
   ~/project/crow/crow/gates/udpgate.cpp \
   ~/project/crow/crow/proto/node.cpp \
   ~/project/crow/crow/proto/node-sync.cpp \
   ~/project/crow/crow/src/hostaddr.cpp \
   ~/project/igris/igris/sync/syslock_mutex.cpp \
   ~/project/igris/igris/osutil/src/posix.cpp \
   ~/project/igris/igris/util/string.cpp \
   ~/project/igris/igris/osutil/realtime.c \
   ~/project/igris/igris/osinter/wait.c \
   ~/project/igris/igris/osinter/wait-linux.cpp \
   ~/project/igris/igris/dprint/dprint_func_impl.c \
   ~/project/igris/igris/dprint/dprint_stdout.c \
   ~/project/igris/igris/string/replace.cpp \
   ~/project/igris/igris/string/replace_substrings.c \
   ~/project/igris/igris/string/memmem.c \
   /home/mirmik/Android/Sdk/ndk/22.1.7171670/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/26/libc++.a \
    -Wl,--end-group 

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
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/libopencv_core.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/libopencv_imgcodecs.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a/libopencv_imgproc.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibopenjp2.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libittnotify.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libtbb.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibjpeg-turbo.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libtegra_hal.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/libIlmImf.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibwebp.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibpng.a
aapt add hello_quest.apk /home/mirmik/Downloads/OpenCV-android-sdk/sdk/native/3rdparty/libs/arm64-v8a/liblibtiff.a

apksigner\
	sign\
	-ks ~/.android/debug.keystore\
	--ks-key-alias androiddebugkey\
	--ks-pass pass:android\
	hello_quest.apk
popd > /dev/null
